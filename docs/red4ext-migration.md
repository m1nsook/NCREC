# RED4ext migration — stage 1 (safe bootstrap)

The initial native DX12/ASI proof of concept has shown input and render
instability during active gameplay. **Do not rename NCREC.asi to NCREC.dll.**
RED4ext requires its own exported plugin API and explicit lifecycle.

## Branch and installed files

Development branch: `feature/red4ext-host`.

- Old experiment: `bin/x64/plugins/NCREC.asi` (Ultimate ASI Loader).
- New bootstrap: `red4ext/plugins/NCREC/NCREC.dll` (RED4ext runtime).

**Never install both during testing.** They are separate loaders, but the
same project would be initialized twice if both were enabled. Move the ASI
out of the game's plugin directory before testing the RED4ext DLL.

## What the DLL does in stage 1

- Implements RED4ext **API v1** `Supports`, `Query` and `Main` exports
  using the pinned 1.0.0 SDK.
- Sends load/unload diagnostics to the RED4ext logger.
- Does not install a DirectX hook, create another window, acquire input or
  access/modify game memory.
- Deliberately uses `RED4EXT_V1_RUNTIME_VERSION_INDEPENDENT` **only** because
  the bootstrap does not touch REDengine internals. This must change to a
  tested game-version gate before introducing active engine integration.
- Keeps all camera-path math, keyframes and JSON save/load source code intact.

The official 1.0.0 SDK runtime header currently lists
`RED4EXT_V1_RUNTIME_VERSION_LATEST` as game 2.31, not the 2.5 game
version observed during NCREC testing. No unverified 2.5 game offsets,
RTTI calls or camera hooks are used by this bootstrap.

## Build

Windows x64 (Visual Studio 2022, CMake 3.24+):

```powershell
cmake -S . -B build-red4ext -A x64 -DNCREC_BUILD_PLUGIN=OFF -DNCREC_BUILD_RED4EXT=ON
cmake --build build-red4ext --config Release --target NCREC_RED4ext
```

Output: `build-red4ext/red4ext/plugins/NCREC/Release/NCREC.dll`.

Copy `NCREC.dll` to `Cyberpunk 2077/red4ext/plugins/NCREC/`.
Use the game-version-compatible **RED4ext loader**; the SDK header
is a compile-time dependency, not a replacement for the installed loader.

## Fullscreen editor contract (from demo.html)

The requested interface is a fixed, fullscreen application shell:

- Top menu (36px), left Project/Shots (260px),
  center Preview with transport (44px), right Inspector (320px),
  bottom Timeline (240px), and Status (26px).
- The root has **no window decoration, dragging or global scrollbars**.
- Only the shot list, console and long settings may scroll.
- On narrow displays the sidebars collapse to preserve the preview.
- The portable `calculateWorkspace()` function computes all panel
  coordinates without requiring ImGui to be initialized.

The new RED4ext DLL **does not display this interface yet**. Activating
a renderer before verifying cursor ownership, pause-mode boundaries and
DX12 lifecycle would repeat the instability that was reported.

## Acceptance gates before enabling the editor

1. Verify the RED4ext DLL loads/unloads cleanly on Cyberpunk 2.5, with logs.
2. Validate game lifecycle timing and supported camera APIs.
3. Establish game-aware cursor input ownership, preserve Alt+F4 and emergency
   Escape/recovery controls; do not swallow system messages.
4. Bring up DX12 rendering with safe initialization and shutdown, tested
   alongside CET/ReShade/OBS.
5. Add the fullscreen workspace from `demo.html` with no unwanted scrollbars.
6. Only then enable real camera read/override/restore in a supported version.

The DLL host is not proof that the in-game editor or camera already works.
