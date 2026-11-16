<img width="250" height="126" alt="ncrec-logotype" src="https://github.com/user-attachments/assets/708117dd-40b8-458c-8310-0219b7f2c154" />

# NCREC
Cinematic camera editor for Cyberpunk 2077, inspired by HLAE's mirv_campath.

> **Experimental DX12 renderer branch.** This build replaces the separate
> Win32 editor window with an in-game Dear ImGui layer that renders directly
> into Cyberpunk's DX12 swapchain. **No Cyberpunk camera control yet.**
> This branch has NOT been validated in the game and could cause incompatibilities.

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
- One-key editor toggle (grave key by default) with in-editor rebinding in
  `bin/x64/plugins/NCREC.ini`. Escape and the Hide UI button also close it.
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
4. Start the game, press **grave / tilde** (left of 1 on most keyboards) to
   toggle the NCREC overlay. No Ctrl, Shift or Fn needed. Change the binding
   in the UI if necessary.
5. Use `path demo` then `path play` to test timeline evaluation only.
6. Test a desktop screenshot with NCREC visible. A DX12 capture pipeline
   should include the overlay if capturing the final frame, but **some
   recording/capture integrations may hook earlier and omit it**.

The UI no longer automatically hides on Alt+Tab. It belongs to the game
framebuffer, so when the game stops rendering while out of focus, the last
rendered frame remains visible; it is not a separate desktop window.

**Known limitations:** The camera adapter, recording integration and project
save/load are not implemented. This experimental hook intercepts DXGI APIs and
may conflict with CET/ReShade/other injectors. Test carefully and report logs
and game version. This renderer has not been tested in Cyberpunk yet; CI only
proves that the code compiles and core tests pass. See
[architecture](docs/architecture.md), [roadmap](docs/roadmap.md), and
[third-party notices](docs/third_party.md).

## License

MIT. See [LICENSE](LICENSE). Vendored DX12 hook license is in
[src/platform/dx12/LICENSE](src/platform/dx12/LICENSE).
