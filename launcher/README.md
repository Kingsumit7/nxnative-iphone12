# NXNative Launcher for macOS

NXNative Launcher is a native macOS application for managing game sources and building unsigned iOS IPAs with the NXNative repository.

It runs on the Mac. It is not an iOS launcher and does not run games itself.

## Features

The current implementation provides:

- A persistent library of game-source paths. Large source files remain in their original locations.
- Support for an NSP folder, a supported NUP container, or an extracted game directory.
- Selectors for the NXNative repository, an optional `prod.keys` file, and the IPA output directory.
- A build action that invokes the repository's existing Python build script.
- Live build logs and structured progress events.
- Build cancellation and exit-status reporting.
- Unsigned IPA output.

## Build pipeline

The launcher invokes the repository's existing build entry point:

```sh
python3 scripts/build_ipa_from_nsp.py <game-source> --platform ios --progress json -o <output.ipa>
```

For encrypted NSP input, select the folder containing the game's NSP files and provide the user's own keys file using the script's `-k` option. Supported NUP containers and extracted dump folders do not require a keys file. A single standalone NSP file is not a supported input.

The app starts Python with an argument array. It does not interpolate user paths into a shell command. The build script writes progress events as newline-delimited JSON, which the launcher uses to update its progress display.

## Requirements

- macOS 13 or newer
- Xcode and Xcode command-line tools
- XcodeGen
- Python 3.12 or newer
- The Python dependencies and native build tools listed by `scripts/build_ipa_from_nsp.py`
- An NXNative repository checkout containing the build script

The launcher does not bundle Python or the NXNative build environment. Configure the app to use a local repository checkout and install the build dependencies before starting a game build.

## Build the macOS application

From the repository root:

```sh
brew install xcodegen
xcodegen generate --spec launcher/project.yml --project launcher
xcodebuild \
  -project launcher/NXNativeLauncher.xcodeproj \
  -scheme NXNativeLauncher \
  -configuration Release \
  -destination 'generic/platform=macOS' \
  -derivedDataPath build/DerivedData \
  CODE_SIGNING_ALLOWED=NO \
  build
```

The app bundle is written to:

```
build/DerivedData/Build/Products/Release/NXNative Launcher.app
```

GitHub Actions runs the same build and uploads a ZIP artifact named `NXNative-macOS-Launcher`.

## A14 and ASTC compatibility

The launcher selects the iOS build target. It does not make the runtime compatible by itself.

A14 support requires a graphics path that works on Apple A14 GPUs. This includes real BC texture decoding and ASTC encoding or another supported fallback, preserving texture data and mipmaps, and replacing Metal 4-only runtime paths with implementations supported by A14 devices.

BC-to-ASTC conversion is not implemented by this launcher yet. Do not treat a successful launcher build, IPA package, or texture conversion test as proof that a game runs correctly. Validate the resulting IPA on an iPhone 12 before making compatibility claims.

## Signing and installation

The launcher produces unsigned IPAs. It does not store signing certificates, sign applications, or install applications on a device. Users control their own installation process.

## Current limitations

- The launcher is an initial build interface. It does not decrypt game data itself. The NXNative build script handles supported input formats.
- The launcher requires a configured local Python environment and NXNative checkout.
- It does not currently convert BC textures to ASTC.
- A14 Metal fallback work and physical-device game testing are still outstanding.
