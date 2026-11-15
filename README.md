# NXNative iPhone 12

An experimental iOS-focused fork of [nxnative](https://github.com/seedds/nxnative), exploring a dedicated game-library launcher and a path toward support for iPhone 12 and newer Apple devices.

This repository is under active development. The iPhone 12 launcher and game-runtime compatibility work are separate milestones. A working launcher interface does not, by itself, make the underlying game runtime compatible with older iPhones.

## Project status

- Target device: iPhone 12, powered by Apple A14 Bionic.
- Initial direction: develop a native iOS launcher with a game library and a clear path to the NXNative runtime.
- Build environment: GitHub Actions, with device builds intended for sideloading.
- Current state: prototype planning and compatibility investigation. The dedicated launcher has not yet been implemented, and iPhone 12 game execution has not been validated.

Do not treat this fork as a working iPhone 12 game launcher yet.

## Compatibility

The upstream iOS project currently has requirements that prevent it from being considered iPhone 12 compatible:

- Its current iOS project targets iOS 26.
- Parts of its graphics path use newer Metal functionality.
- The documented device requirements specify A17 Pro or later for iPhone.
- The current texture and graphics pipeline includes requirements that older iPhone GPUs may not satisfy.

The A14 target therefore requires runtime and graphics investigation, not only a change to the deployment target or compiler flags. Any compatibility claim will be updated only after successful builds and testing on real hardware.

## Development goals

The work is organized into independent milestones:

1. **Launcher foundation**: create a native iOS app shell and game-library interface.
2. **Library management**: define safe game-import, metadata, and local-library workflows.
3. **Runtime integration**: connect the launcher to the existing NXNative build and launch flow where the architecture permits.
4. **A14 compatibility**: identify and address deployment, CPU, memory, and Metal constraints in the engine.
5. **Validation**: build through GitHub Actions, install through a user-controlled sideloading workflow, and test on iPhone 12 hardware.

A milestone is complete only when its implementation and validation are documented.

## Build status

GitHub Actions is the intended build environment. Workflow support and reproducible iOS builds are still part of the development work. Until a workflow is added and verified, this repository does not promise a downloadable or installable IPA.

iOS builds should remain unsigned for user-controlled sideloading. This project will not sign or install software on a user's device.

## Upstream project

This fork builds on the architecture and source code of [seedds/nxnative](https://github.com/seedds/nxnative). Upstream documentation remains the reference for the game runtime, game profiles, graphics implementation, and development procedures:

- [Upstream README](https://github.com/seedds/nxnative)
- [Development guide](https://github.com/seedds/nxnative/blob/main/docs/development.md)
- [Front-end integration guide](https://github.com/seedds/nxnative/blob/main/docs/frontends.md)
- [Game compatibility notes](https://github.com/seedds/nxnative/tree/main/dm/games)

## Contributing

Keep changes focused and document their purpose, affected components, and validation results. Do not claim device compatibility based only on a successful compile. Include relevant build logs and device-test details when reporting issues.

Before changing a subsystem, read the repository's [AGENTS.md](AGENTS.md) and the relevant upstream development guide.

## Legal

This project is an independent community effort and is not affiliated with, endorsed by, or sponsored by Nintendo. Users are responsible for providing their own legally obtained game files. No game files, encryption keys, or copyrighted game assets belong in this repository.

See [LICENSE](LICENSE) for the repository license. Third-party components retain their respective licenses.
