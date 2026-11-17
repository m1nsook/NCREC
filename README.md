<img width="250" height="126" alt="ncrec-logotype" src="https://github.com/user-attachments/assets/708117dd-40b8-458c-8310-0219b7f2c154" />

# NCREC
Cinematic camera-path editor for Cyberpunk 2077.

> **Source of truth: [main](https://github.com/m1nsook/NCREC/tree/main).**
> This project is **in active development, not a stable in-game release**.
> The native RED4ext DLL currently **logs startup/shutdown only**; it does
> not draw a UI, take over cursor input, or modify the game camera.
> The old ASI renderer is retained in source solely as an **unstable legacy
> experiment**. Don't install both.

## Find your files

| What you're looking for | Path on `main` |
| --- | --- |
| New RED4ext DLL entry point | `src/platform/red4ext/Plugin.cpp` |
| Fullscreen ImGui shell and layout geometry | `src/editor/FullscreenEditor.*`, `src/editor/WorkspaceLayout.*` |
| Camera paths, keyframes, JSON projects | `src/core/` |
| Your fullscreen UI reference | `design/demo.html` |
| Previous, unstable ASI/ImGui UI | `src/platform/GameOverlay.cpp` |
| RED4ext migration and validation | `docs/red4ext-migration.md` |

**Repository policy:** Completed, CI-tested work goes to `main`.
New features use a short-lived `feature/*` branch and one reviewable PR;
experimental gameplay hooks remain **off** until verified in Cyberpunk.
See [development workflow](docs/REPOSITORY.md).

## Build the safe RED4ext DLL

Windows x64 / Visual Studio 2022 / CMake 3.24+:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target NCREC_RED4ext
ctest --test-dir build -C Release --output-on-failure
```

The DLL lands in `build/red4ext/plugins/NCREC/Release/NCREC.dll` and
requires a compatible RED4ext loader installed with Cyberpunk.
**Remove `bin/x64/plugins/NCREC.asi` before testing the DLL.**
Only RED4ext load/unload log messages are expected right now.

The old ASI remains *opt-in* for developers only:
`-DNCREC_BUILD_PLUGIN=ON -DNCREC_BUILD_RED4EXT=OFF`.
**Do not distribute it as the stable editor.**

## Goals

- **Dedicated UI and console**, independent of CET.
- Keyframed cinematic camera paths with precise editing and interpolation.
- Save/load project files and repeatable shots.
- Optional OBS WebSocket and a future OBS Studio frontend plugin.

## Current prototype

- Cross-platform C++20 camera path interpolation: cubic Hermite/linear
  positions, quaternion SLERP, FOV blending.
- **Virtual camera rig**: numeric position/FOV editing, camera rotation commands,
  timed keyframe capture/removal, and scrubbed playback without accessing the
  live game view.
- **Camera path projects**: save/load named `*.ncrec.json` files via the
  Timeline page or `path save <name>` / `path load <name>` commands.
  Files are saved under `bin/x64/plugins/NCREC/projects/`.
- `help`, `status`, `camera pose`, `camera pos <x> <y> <z>`,
  `camera fov <degrees>`, `camera rotate <yaw> <pitch> <roll>`,
  `path demo`, `path add <seconds>`, `path remove <index>`,
  `path list`, `path play`, `path pause`, `path seek <seconds>`,
  `path sample <seconds>`, `path save <name>`, `path load <name>`
  and `path clear`.
- Experimental Windows x64 ASI with **actual in-game DX12 ImGui renderer**,
  not an always-on-top secondary Win32 window.
- On a **fresh installation**, an automatic in-game welcome screen asks
  for a single toggle key; no default shortcut or multi-key combination.
  Press one key, review the selection and click **Save and finish**.
- Existing named-key settings migrate automatically; saved keys are retained.
  Change the key later in the **Settings** tab.
- Modern dark interface with Inter for UI and JetBrains Mono for the console,
  both **embedded in NCREC.asi**. Appearance settings include UI scaling.
- Separate **Timeline**, **Console** and **Settings** views. Escape and
  **Hide UI** close the main editor after setup.
- Unit tests and Windows/Linux CI.

## Build

Windows 10/11, Visual Studio 2022 C++ tools and CMake 3.24+.
Configuration requires network access for pinned Dear ImGui and MinHook.

```powershell
cmake -S . -B build -A x64 -DNCREC_BUILD_PLUGIN=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Output: `build/bin/x64/plugins/Release/NCREC.asi`

Core-only tests without Windows or Cyberpunk:

```sh
cmake -S . -B build -DNCREC_BUILD_PLUGIN=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

## Experimental installation

1. Back up the current working NCREC ASI before trying the DX12 branch.
2. Install/retain an x64 compatible ASI loader, such as Ultimate ASI Loader,
   only if your setup needs it; avoid overwriting existing mod loaders.
3. Copy the built `NCREC.asi` to `<Cyberpunk 2077>/bin/x64/plugins/`.
4. On your **first launch without a saved bind**, NCREC opens a centered
   welcome window automatically. Press a single key and select
   **Save and finish**. The wizard closes.
5. Press the chosen key to reopen NCREC. Use **Settings** to change the
   key or UI scale, or **Console** to type `path demo` and `path play`
   (math preview only).
6. If you previously used NCREC, your existing `[Hotkeys] Toggle`
   entry should be migrated. New key codes are saved in
   `bin/x64/plugins/NCREC.ini` as `[Hotkeys] ToggleVK`.
7. Test a desktop screenshot with NCREC visible. A DX12 capture pipeline
   should include the overlay if capturing the final frame, but **some
   capture integrations may hook earlier and omit it**.

The UI no longer automatically hides on Alt+Tab. It belongs to the game
framebuffer, so when the game stops rendering while out of focus, the last
rendered frame remains visible; it is not a separate desktop window.

**Known limitations of the legacy ASI:** Live camera access and recording integration are not implemented. JSON path save/load is implemented for the virtual camera. This experimental hook intercepts DXGI APIs and
may conflict with CET/ReShade/other injectors. Test carefully and report logs
and game version. The new onboarding UI/font integration needs game validation. CI only
proves that the code compiles and core tests pass. See
[architecture](docs/architecture.md), [UI design](docs/ui-design.md),
[roadmap](docs/roadmap.md), and [third-party notices](docs/third_party.md).

## License

NCREC source: MIT; see [LICENSE](LICENSE). Vendored DX12 hook license is in
[src/platform/dx12/LICENSE](src/platform/dx12/LICENSE). Embedded font
licenses: [Inter OFL](licenses/Inter-OFL.txt) and
[JetBrains Mono OFL](licenses/JetBrainsMono-OFL.txt).

### Emergency recovery and mouse capture

The first-run wizard has a keyboard-only fallback: after choosing a key,
press **Enter** to save. **Esc** hides the overlay in an emergency, and **F10**
reopens first-run setup if no key has been saved. **Alt+F4** is passed through to
Cyberpunk rather than intercepted by NCREC. The input layer also attempts to
unclip the game's mouse and use relative RawInput if the pointer stays locked.
This behavior is experimental and requires real-game testing.

If NCREC makes the game unresponsive, remove `NCREC.asi` from
`bin/x64/plugins/` before launching again. Do not use the prior trapped-input
build.

## Virtual camera workflow (new experiment)

This build lets you **practice creating camera paths before NCREC is connected
with Cyberpunk's actual camera**. In Timeline, enter X/Y/Z and FOV, select
a timestamp (0 for the first point, 3.0 for the next), then click
**Add virtual pose**. Add more timed points, scrub or play the preview,
then save the shot. The editor includes **Save / Load Shot** using a simple
file name such as `intro_pan`. The path is stored as JSON so it can be
shared with other editors. The game view **will not move** in this version.

For actual camera reads/overrides, see [camera adapter design](docs/camera-adapter.md).
