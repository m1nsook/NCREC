<img width="250" height="126" alt="ncrec-logotype" src="https://github.com/user-attachments/assets/708117dd-40b8-458c-8310-0219b7f2c154" />

# NCREC
Cinematic camera editor for Cyberpunk 2077, inspired by HLAE's mirv_campath.

> **Early development / v0.1 prototype.** The current build does **not**
> control Cyberpunk's camera yet. Do not expect working cinematic capture.

## Goals

- A **dedicated interface and console**, independent of the CET overlay.
- Timestamped camera paths with precise editing and smooth interpolation.
- Reliable save/load and repeatable cinematic camera motion.
- Eventually, optional OBS Studio recording automation and plugin support.

## Current prototype

- Portable C++20 camera path model: time-aware cubic Hermite/linear positions,
  shortest-arc quaternion rotations and FOV interpolation.
- A command/timeline preview with `help`, `status`, `path demo`,
  `path list`, `path play`, `path pause`, `path seek <seconds>`,
  `path sample <seconds>` and `path clear`.
- Windows x64 `NCREC.asi` with its **own Win32/D3D11 Dear ImGui editor window**.
  **This is a separate window, not a DX12 injection/game renderer hook.**
- Unit tests and GitHub Actions CI.

## Build

Requirements: Windows 10/11, Visual Studio 2022 C++ tools, CMake 3.24+,
internet access at configure time to fetch a pinned Dear ImGui revision.

```powershell
cmake -S . -B build -A x64 -DNCREC_BUILD_PLUGIN=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Output: `build/bin/x64/plugins/Release/NCREC.asi`

Core-only development is supported on Linux/macOS:

```sh
cmake -S . -B build -DNCREC_BUILD_PLUGIN=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

## Experimental installation

1. Use Cyberpunk 2077 in **borderless or windowed mode**.
2. Install the **x64 Ultimate ASI Loader** as `bin/x64/version.dll` if you do
   not already have a compatible ASI loader. CET itself is **not required**.
   Avoid overwriting an existing loader without checking your setup.
3. Copy `NCREC.asi` to `<Cyberpunk 2077>/bin/x64/plugins/NCREC.asi`.
4. Launch Cyberpunk. Press the **grave / tilde key** (usually immediately
   left of `1`) **once** to open/close NCREC. No Ctrl, Shift, Alt or Fn.
   In the editor, use **Focus console** to enter commands, **Esc** to close,
   or click **Close editor**. If CET or another mod already uses the grave key,
   the prototype tries **Insert** as a fallback.
   Choose a different **single key** under `Toggle key (one press)` in the
   NCREC window. Your choice is saved in `bin/x64/plugins/NCREC.ini`.
   Some keys, such as Caps Lock, may conflict with in-game controls.
5. Check `bin/x64/plugins/NCREC.log` for bootstrap logs. In the console,
   run `path demo` and `path play` to preview **math only**, not the camera.

**Compatibility note:** The initial UI is a separate topmost Windows window
created inside the game process. It is not part of Cyberpunk's DX12 backbuffer,
does not support exclusive-fullscreen rendering, and has not yet been validated
against game versions or other overlays. The game camera, OBS integration,
recording, save/load and frame stepping are **not implemented**.

See [architecture](docs/architecture.md) and [roadmap](docs/roadmap.md).

## License

MIT. See [LICENSE](LICENSE).
