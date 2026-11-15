# NXNative for iPhone 12 and Newer

NXNative is an experimental Apple-platform port of [seedds/nxnative](https://github.com/seedds/nxnative). This fork focuses on building unsigned iOS game packages and developing runtime support for iPhone 12 (Apple A14 Bionic) and newer devices.

The project has two separate deliverables:

- **NXNative Launcher for macOS:** a Mac application that manages game sources and runs the build pipeline.
- **Unsigned iOS game IPA:** the output generated for a selected game. The Mac launcher is a build tool, not an iOS launcher.

## Project status

The initial macOS launcher implementation is in place. It provides a game-source library, build configuration, progress output, cancellation, and unsigned IPA output through the existing NXNative build script. GitHub Actions builds and packages the Mac application.

A14 runtime compatibility remains in development. The launcher can invoke the existing iOS build pipeline, but that does not mean the resulting game already runs on iPhone 12. BC-to-ASTC texture conversion, an A14-compatible Metal execution path, and physical-device validation remain outstanding.

## Project goals

1. Maintain a native macOS interface for the NXNative game build pipeline.
2. Manage NSP folders, supported NUP files, and extracted game sources without copying large files unnecessarily.
3. Build unsigned iOS IPAs and report progress, logs, and errors.
4. Add a tested A14-compatible graphics path, including conversion of unsupported BC-compressed textures to ASTC where format and usage permit.
5. Replace or provide compatible alternatives for runtime features that require newer Metal APIs.
6. Validate game execution on physical A14 hardware before declaring compatibility.

## A14 compatibility

A successful compile or IPA package does not prove that a game runs on iPhone 12. Compatibility work must cover the complete runtime and asset pipeline.

The current upstream iOS target uses newer platform and graphics functionality. A14 support therefore requires more than changing the deployment target or compiler flags.

The compatibility work covers:

- **Texture formats:** identify BC-compressed inputs and implement a tested conversion path to ASTC or another supported representation. Preserve mip levels, channel layout, alpha, and texture semantics. Do not relabel BC data as ASTC.
- **Metal API availability:** provide an A14-compatible implementation for functionality that currently depends on Metal 4.
- **CPU target:** audit compiler flags and generated instructions for A14 compatibility.
- **Deployment target:** select an iOS minimum version supported by the implementation.
- **Device validation:** test startup, rendering, input, audio, saves, and sustained gameplay on an actual iPhone 12.

A14 support remains unverified until these areas pass implementation tests and physical-device validation. Compatibility may vary by game.

## macOS launcher

The launcher runs on macOS. Add an NSP folder, supported NUP file, or extracted game directory. Select the NXNative repository, an optional keys file, and an output folder. The launcher invokes `scripts/build_ipa_from_nsp.py`, displays build progress and diagnostics, and writes an unsigned IPA.

The launcher does not sign or install the IPA. Users remain responsible for their own installation workflow.

See [launcher/README.md](launcher/README.md) for requirements, build instructions, and current limitations.

## Building an iOS game IPA manually

The repository's existing build entry point is:

```sh
python3 scripts/build_ipa_from_nsp.py <game-source> --platform ios -o <output.ipa>
```

For encrypted NSP input, provide your own keys file with `-k <prod.keys>`. The source is normally a folder containing the game's NSP files. Supported NUP containers and extracted dump folders follow the input rules documented by the build script.

Install the repository's required Python packages and Apple build tools before running it. Consult [docs/development.md](docs/development.md) and the script's command-line help for current requirements and options.

Keep game files, keys, and extracted content outside the repository. Never commit game dumps, encryption keys, generated IPAs, or copyrighted game assets.

## Development and validation

Read [AGENTS.md](AGENTS.md) before modifying the engine or build pipeline. Use the upstream documentation for architecture and subsystem-specific guidance:

- [Upstream NXNative](https://github.com/seedds/nxnative)
- [Development guide](https://github.com/seedds/nxnative/blob/main/docs/development.md)
- [Front-end integration](https://github.com/seedds/nxnative/blob/main/docs/frontends.md)
- [Game profiles and compatibility notes](https://github.com/seedds/nxnative/tree/main/dm/games)

Every change should include relevant automated checks. Describe a successful compile as a build result, not proof of device compatibility. Record the physical device, iOS version, game version, and observed results when reporting hardware tests.

## Legal

This project is an independent community effort. It is not affiliated with, endorsed by, or sponsored by Nintendo. Users must provide their own legally obtained game files. No game files, encryption keys, or copyrighted game assets are distributed by this repository.

See [LICENSE](LICENSE) for license terms. Third-party components remain subject to their respective licenses.
