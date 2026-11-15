# NXNative Launcher for macOS

NXNative Launcher is planned as a native macOS application for managing game sources and building unsigned iOS IPAs with the NXNative repository.

It runs on the Mac. It is not an iOS launcher and does not run games itself.

## Intended workflow

1. Add an NSP, NUP, or supported extracted game directory to the library.
2. Select a game and configure the required source files, including a keys file when the source format requires one.
3. Start the iOS build.
4. Follow progress and diagnostic output in the launcher.
5. Save the generated unsigned IPA to a chosen location.

The launcher should reference large game files in place where possible. It must not expose key contents in logs or copy game data into the Git repository.

## Build pipeline

The launcher is designed to call the repository's existing build entry point:

```sh
python3 scripts/build_ipa_from_nsp.py <game-source> --platform ios --progress json -o <output.ipa>
```

For encrypted NSP input, pass the user's own keys file using the script's `-k` option. NUP and extracted dump inputs follow the input requirements documented by the build script.

The Mac application must invoke the script using an argument array, not a shell-interpolated command string. It should display standard output and errors, parse progress events, support cancellation, and report the final exit status.

## Requirements

The completed launcher will require:

- macOS and Xcode command-line tools
- Python 3.12 or newer, subject to the build script's current requirements
- The Python dependencies and native build tools documented by the repository
- An NXNative source checkout containing `scripts/build_ipa_from_nsp.py`

The exact minimum macOS version will be set by the Xcode project and documented after the native macOS target is implemented.

## A14 and ASTC compatibility

The launcher selects the iOS build target. It does not make the runtime compatible by itself.

The build and runtime work must provide an A14-compatible graphics path. This includes converting BC-compressed textures to ASTC or another supported representation when appropriate, preserving texture data and mipmaps, and replacing Metal 4-only runtime paths with implementations supported by A14 devices.

Do not treat a successful launcher build, IPA package, or texture conversion command as proof that a game runs correctly. Validate the resulting IPA on an iPhone 12 before making compatibility claims.

## Signing and installation

The launcher produces unsigned IPAs. It does not store signing certificates, sign applications, or install applications on a device. Users control their own installation process.

## Current status

This document describes the intended macOS launcher workflow. The existing iOS-only launcher prototype is not the target product and must not be presented as the completed Mac launcher. The macOS application, A14 graphics path, BC-to-ASTC conversion, and physical-device validation remain implementation tasks until their code and tests are in place.
