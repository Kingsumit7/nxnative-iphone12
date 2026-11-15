"""The iOS app's CPU target is Apple A14, the oldest supported device class.

dav1d (dm/third_party/dav1d/src/cpu.h) adds every CPU feature promised by the compiler target to the features it detects at run time. A target newer than
the device can therefore execute unsupported instructions. A14 provides the LSE atomics required by the host assembly without enabling newer optional
instruction sets such as I8MM or BF16. Keep the target aligned with the iPhone 12 minimum and test the compiler's feature macros here."""
import re
import shutil
import subprocess
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
# A14 must provide LSE atomics for the host assembly and must not promise newer optional instruction sets.
NEEDED = ('__ARM_FEATURE_ATOMICS',)
ABSENT = ('__ARM_FEATURE_MATMUL_INT8', '__ARM_FEATURE_BF16_VECTOR_ARITHMETIC', '__ARM_FEATURE_SME')


def macros(mcpu):
    out = subprocess.run(['xcrun', '-sdk', 'iphoneos', 'clang', '-target', 'arm64-apple-ios26.0', f'-mcpu={mcpu}', '-dM', '-E', '-x', 'c', '/dev/null'],
                         capture_output=True, text=True, check=True).stdout
    return {line.split()[1] for line in out.splitlines() if line.startswith('#define ')}


class CpuTarget(unittest.TestCase):
    def targets(self):
        project = (REPO / 'dm/ios/project.yml').read_text()
        check = (REPO / 'dm/ios/tools/compile_check.py').read_text()
        found = {'project.yml': re.findall(r'-mcpu=([\w-]+)', project), 'compile_check.py': re.findall(r"-mcpu=([\w-]+)", check)}
        for where, cpus in found.items(): self.assertEqual(len(cpus), 1, f'{where}: one -mcpu, found {cpus}')
        return {where: cpus[0] for where, cpus in found.items()}

    def test_ios_target_promises_no_feature_the_oldest_supported_chip_lacks(self):
        if not shutil.which('xcrun'): self.skipTest('Xcode required')
        for where, cpu in self.targets().items():
            with self.subTest(where=where, cpu=cpu):
                m = macros(cpu)
                for feature in NEEDED: self.assertIn(feature, m, f'{where}: -mcpu={cpu} lacks {feature} (LSE), which the host assembly uses')
                for feature in ABSENT: self.assertNotIn(feature, m, f'{where}: -mcpu={cpu} promises {feature}, which A14 lacks')

    def test_both_builds_use_the_a14_target(self):
        targets = self.targets()
        self.assertEqual(len(set(targets.values())), 1)
        for where, cpu in targets.items():
            with self.subTest(where=where):
                self.assertEqual(cpu, 'apple-a14')


if __name__ == '__main__': unittest.main()
