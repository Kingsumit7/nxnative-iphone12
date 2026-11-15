# NXNative for iPhone 12 and Newer

NXNative is an experimental Apple-platform port of [seedds/nxnative](https://github.com/seedds/nxnative). This fork focuses on building unsigned iOS game packages that can run on iPhone 12 (Apple A14 Bionic) and newer devices.

The project has two distinct deliverables:

- **NXNative Launcher for macOS:** a Mac application for managing game sources and running the build pipeline.
- **Unsigned iOS game IPA:** the output produced for a selected game. The Mac launcher is a build tool, not an iOS launcher.

## Project goals

1. Provide a native macOS interface for the existing NXNative game build pipeline.
2. Import and manage NSP, NUP, and supported extracted game sources without duplicating large files unnecessarily.
3. Build an unsigned iOS IPA for the selected game and report build progress, logs, and errors.
4. Add an A14-compatible graphics path, including conversion of unsupported BC-compressed textures to an A14-supported ASTC representation where format and usage permit.
5. Replace or provide compatible alternatives for runtime features that require newer Metal APIs.
6. Validate the result on physical A14 hardware before declaring a game or the platform supported.

## A14 compatibility

A successful compile or IPA package does not prove that a game runs on iPhone 12. Compatibility work must cover the complete runtime and asset pipeline.

The current upstream iOS target uses newer platform and graphics functionality. A14 support therefore requires more than lowering the deployment target.

The compatibility work is tracked across these areas:

- **Texture formats:** identify BC-compressed inputs and add a tested conversion path to ASTC or another supported format. Preserve mip levels, channel layout, alpha, and texture semantics. Do not blindly relabel BC data as ASTC.
- **Metal API availability:** provide an A14-compatible implementation for functionality that currently depends on Metal 4.
- **CPU target:** audit compiler flags and generated instructions for A14 compatibility.
- **Deployment target:** select an iOS minimum version supported by the implementation, not simply a lower project setting.
- **Device validation:** test startup, rendering, input, audio, saves, and sustained gameplay on an actual iPhone 12.

A14 support remains a development goal until these areas pass implementation tests and physical-device validation. Compatibility may vary by game.

## macOS launcher

The launcher is intended to run on macOS. It will select a game source, invoke the repository's existing build script, display progress and diagnostics, and save the resulting unsigned IPA to a location chosen by the user.

The launcher must not attempt to install or sign the IPA. Users remain responsible for their own installation workflow.

See [launcher/README.md](launcher/README.md) for launcher design and build instructions.

## Building an iOS game IPA manually

The repository's existing build entry point is:

```sh
python3 scripts/build_ipa_from_nsp.py <game-source> --platform ios -o <output.ipa>
```

For encrypted NSP sources, provide your own keys file with `-k <prod.keys>`. Extracted dump folders and supported NUP sources follow the input rules documented by the build script. Install the repository's required Python packages and Apple build tools before running it. Consult [docs/development.md](docs/development.md) and the script's command-line help for the current requirements and options.

Keep game files, keys, and extracted content outside the repository. Never commit game dumps, encryption keys, generated IPAs, or copyrighted game assets.

## Development and validation

Read [AGENTS.md](AGENTS.md) before modifying the engine or build pipeline. Use the upstream documentation for architecture and subsystem-specific guidance:

- [Upstream NXNative](https://github.com/seedds/nxnative)
- [Development guide](https://github.com/seedds/nxnative/blob/main/docs/development.md)
- [Front-end integration](https://github.com/seedds/nxnative/blob/main/docs/frontends.md)
- [Game profiles and compatibility notes](https://github.com/seedds/nxnative/tree/main/dm/games)

Every change should include relevant automated checks. A build-only result must be described as a build result, not as proof of device compatibility. Record physical-device model, iOS version, game version, and observed results when reporting hardware tests.

## Legal

This project is an independent community effort. It is not affiliated with, endorsed by, or sponsored by Nintendo. Users must provide their own legally obtained game files. No game files, encryption keys, or copyrighted game assets are distributed by this repository.

See [LICENSE](LICENSE) for license terms. Third-party components remain subject to their respective licenses.
