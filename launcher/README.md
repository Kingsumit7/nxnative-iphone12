# iOS launcher prototype

This directory contains a standalone SwiftUI app target. It is separate from `dm/ios`, whose existing app is generated for one prepared game and currently has newer platform requirements.

## Requirements

- Xcode with the iOS SDK
- XcodeGen
- iOS 17 or later for the launcher interface
- An iPhone or iPad using an arm64 processor

The iOS 17 deployment target is for the SwiftUI launcher only. It does not establish compatibility between the NXNative game runtime and A14 hardware.

## Generate and build

From the repository root:

```sh
brew install xcodegen
xcodegen generate --spec launcher/project.yml --project launcher
xcodebuild \
  -project launcher/NXNativeLauncher.xcodeproj \
  -scheme NXNativeLauncher \
  -configuration Release \
  -sdk iphoneos \
  -destination 'generic/platform=iOS' \
  -derivedDataPath build/DerivedData \
  CODE_SIGNING_ALLOWED=NO \
  build
```

The app bundle is written to:

```
build/DerivedData/Build/Products/Release-iphoneos/NXNative.app
```

## GitHub Actions artifact

The workflow builds the app without signing and packages it as:

```
build/NXNative-unsigned.ipa
```

Open the successful run for the `iOS launcher build` workflow and download the `NXNative-iOS-unsigned-IPA` artifact. The IPA is unsigned. Use a compatible sideloading workflow and your own signing method to install it. The project does not store signing certificates or credentials.

## Current functionality

- Import multiple files through the iOS document picker.
- Copy imported files into the app's Application Support directory.
- Persist a local library index.
- View imported file details.
- Remove imported files from the library.

The prototype currently does not:

- Build or convert game packages.
- Decrypt or extract game content.
- Read game metadata or cover art.
- Launch a game or load the NXNative runtime.
- Claim that an imported game is compatible with iPhone 12.

The file importer is a library-management prototype, not a replacement for the upstream packing pipeline. Runtime integration requires a separate design and validation effort.

## Data location

Imported files are copied into the app sandbox:

`Library/Application Support/Games/`

The JSON library index is stored beside those files. Removing a library entry deletes its copied file. The prototype does not access a user's original file after import.

## Validation

A successful GitHub Actions compile confirms that the launcher target builds with the selected SDK. Successful IPA packaging confirms that the app bundle was archived in an installable IPA layout. Neither result confirms installation, sideloading, import behavior on-device, or game execution. Those require physical-device tests.
