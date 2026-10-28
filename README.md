# FZ

FZ is an Objective-C / UIKit iOS dynamic library project. The source is split into feature-oriented files under `Sources/`, while `SeedLock.m` includes them in order so the existing shared static state and cross-feature helpers remain in one compilation unit.

> **Compatibility note:** this workflow builds an arm64 iOS `.dylib` with the iOS SDK on GitHub Actions. It does not package or inject the library into an app. Runtime behavior depends on the target app, Unity/IL2CPP version, symbols, and permissions.

## Features represented in the source

- **IL2CPP discovery and helpers** — resolves common IL2CPP APIs, searches classes and methods, and caches metadata pointers.
- **Seed controls** — locates `RouteConfig` and applies the configured seed toggle/value.
- **Movement and gameplay controls** — includes character/motor helpers, teleport/physics controls, jump and boundary-related toggles.
- **World and visual options** — contains model transform controls, world configuration, fog/camera/FPS options, ESP-style overlays and material/chams handling.
- **Overlay UI** — UIKit-based floating overlay, menu, and joystick UI.
- **Hooks and macros** — includes runtime hook setup, input macro recording/playback, turbo and related gameplay callbacks.
- **Camera tools** — includes free-camera and camera transform/effect controls.
- **Audio and interface actions** — includes audio controls and menu action wiring.

Feature availability is target-build dependent. Some features require specific class names or methods to exist in the game; missing symbols can make individual options unavailable or cause incompatibility. Test only with apps and game environments you own or are authorized to modify.

## Project layout

```text
SeedLock-Project/
├── SeedLock.m                 # Source aggregator compiled by the workflow
├── Sources/
│   ├── 00_Runtime.inc         # Imports, IL2CPP API resolution, common helpers
│   ├── 10_Gameplay.inc        # Core gameplay state and movement/seed helpers
│   ├── 20_WorldVisuals.inc    # World, visual, ESP and material helpers
│   ├── 30_OverlayAndEngine.inc# Colors, overlay and engine setup
│   ├── 40_HooksAndMacros.inc  # Hooks, audio, macros and turbo helpers
│   ├── 50_Camera.inc          # Camera/free-camera features
│   └── 60_MenuAndEntry.inc    # Menu UI and initialization
├── Resources/
│   ├── Icon.png               # Add your own icon here (optional)
│   └── README.md
├── .github/workflows/build.yml# GitHub Actions workflow
├── build.yml                  # Workflow copy for convenience
└── unzip.sh                   # Extracts project.zip
```

## Build on GitHub

1. Upload the project contents to a GitHub repository.
2. Add your image at `Resources/Icon.png` (capitalization matters on macOS).
3. Open **Actions** and run **Build SeedLock**, or push a commit.
4. Download the `SeedLock-dylib` artifact from the completed workflow.

The generated library is an arm64 iOS dynamic library. The workflow uses ad-hoc signing, which is not a substitute for signing/integration required by a particular deployment method.

## Local extraction

Run the standalone `unzip.sh` beside `project.zip`:

```sh
chmod +x unzip.sh
./unzip.sh
```

By default it extracts `project.zip` into `SeedLock-Project/`. You can also pass paths:

```sh
./unzip.sh /path/to/project.zip /path/to/output-folder
```

## Credits

- [Annette](https://github.com/MeinaWithAI/Annette) — project credit/reference.
