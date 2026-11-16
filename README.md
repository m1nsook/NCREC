<img width="250" height="126" alt="ncrec-logotype" src="https://github.com/user-attachments/assets/708117dd-40b8-458c-8310-0219b7f2c154" />

# NCREC
Cinematic camera editor for Cyberpunk 2077, inspired by HLAE's mirv_campath.

> **Experimental DX12 renderer branch.** This build replaces the separate
> Win32 editor window with an in-game Dear ImGui layer that renders directly
> into Cyberpunk's DX12 swapchain. **No Cyberpunk camera control yet.**
> The base DX12 overlay has been reported working in Cyberpunk 2.5.
> The **new first-launch wizard, settings and embedded fonts still need
> in-game verification** before this draft branch is considered stable.

## Goals

- **Dedicated UI and console**, independent of CET.
- Keyframed cinematic camera paths with precise editing and interpolation.
- Save/load project files and repeatable shots.
- Optional OBS WebSocket and a future OBS Studio frontend plugin.

## Current prototype

- Cross-platform C++20 camera path interpolation: cubic Hermite/linear
  positions, quaternion SLERP, FOV blending.
- `help`, `status`, `path demo`, `path list`, `path play`, `path pause`,
  `path seek <seconds>`, `path sample <seconds>` and `path clear`.
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

**Known limitations:** The camera adapter, recording integration and project
save/load are not implemented. This experimental hook intercepts DXGI APIs and
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
