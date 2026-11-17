# RED4ext migration — stage 1 (safe bootstrap)

The initial native DX12/ASI proof of concept has shown input and render
instability during active gameplay. **Do not rename NCREC.asi to NCREC.dll.**
RED4ext requires its own exported plugin API and explicit lifecycle.

## Branch and installed files

The source is consolidated into the default **`main`** branch after CI checks.
Open feature PRs are work in progress, not another location to hunt for files.

- Old experiment: `bin/x64/plugins/NCREC.asi` (Ultimate ASI Loader).
- New bootstrap: `red4ext/plugins/NCREC/NCREC.dll` (RED4ext runtime).

**Never install both during testing.** They are separate loaders, but the
same project would be initialized twice if both were enabled. Move the ASI
out of the game's plugin directory before testing the RED4ext DLL.

## What the DLL does in stage 1

- Implements RED4ext **API v1** `Supports`, `Query` and `Main` exports
  using the pinned 1.0.0 SDK.
- Sends load/unload diagnostics to the RED4ext logger.
- Refuses to initialize if the legacy `NCREC.asi` is already present in the process.
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

The full-screen ImGui view now exists as `src/editor/FullscreenEditor.cpp` and is **compiled and checked** by the RED4ext build. It is intentionally **not called by the DLL**. Activating a renderer before verifying cursor ownership, pause-mode boundaries and DX12 lifecycle would repeat the instability that was reported.

## Verified game test — 2026-11-17

The owner installed the **NCREC v0.2.0 RED4ext bootstrap** and completed
all six manual checklist items on **Cyberpunk 2077 v2.50 / RED4ext v1.31.1**:

- Prior NCREC.asi removed
- Development ZIP extracted to the game
- Game launches to main menu
- NCREC load message present in RED4ext logs
- Mouse and keyboard remain functional
- Alt+F4 and normal exit function

The logs also explicitly show NCREC's Load and Unload callbacks, and the
RED4ext host loaded and shut down without any logged NCREC errors.

**This validates only the loader-only bootstrap and basic input
non-interference.** It does not validate active-gameplay overlay rendering,
cursor takeover, DX12 hook lifecycle, freecam, or camera writes.

See [test report](test-reports/2026-11-17-red4ext-bootstrap.md).

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

## Stage 2: opt-in passive fullscreen rendering probe (VISUAL RENDERING CONFIRMED; INTERACTIVITY NOT ENABLED)

The C++ fullscreen interface from `design/demo.html` can now be drawn
through the existing D3D12 swapchain **only if the user opts in**. The default
RED4ext startup remains the previously verified, loader-only bootstrap.

For a rendering-only test:

1. Install the newer DLL from the `development-build` prerelease.
2. Copy/rename `red4ext/plugins/NCREC/NCREC-preview.ini.example` to
   `red4ext/plugins/NCREC/NCREC.ini`.
3. Open Cyberpunk 2077 **2.50** and stay at the main menu or pause screen.
4. Press **F8** once to show/hide the fullscreen app shell
   (or your configured `ToggleKey`).
5. Verify screen coverage, panel sizes and that the scene behind the app
   continues to render without flicker or stutters.
6. Verify F8 hides the app and **Alt+F4 still exits**. If the game becomes
   unstable, remove/rename the NCREC.ini and restart. The default DLL then
   returns to loader-only mode.

**This is NOT an interactive editor yet.** The preview:
- does **not** subclass the game's window or intercept WM_INPUT, WM_KEY*
  or WM_SYSKEY*, and does not clip or manipulate the mouse cursor;
- does **not** pause, freeze or take camera control;
- ignores all mouse interaction in ImGui to avoid duplicate clicks that
  might operate the game behind the preview;
- can be toggled with F8 by default (never consumed from the game);
- uses `[Experimental] ToggleKey=F8` in `NCREC.ini` and supports
  F6-F12, Insert, Home, End, PageUp and PageDown as single-key choices;
- does not force a rebinding of CET (which can keep F10);
- uses the same still-experimental DXGI hook family as the old ASI, now
  gated by NCREC.ini and game runtime v2.50;
- can conflict with CET, ReShade or other overlays until tested.

This **must not** be enabled by default before an in-game stability report.
The earlier ASI input trap means the safest next tests are **render-only**
and **menu-first**, followed by validated engine-aware cursor ownership.

## Passive fullscreen screenshot — observed behavior

On 2026-11-17, the project owner ran the RED4ext development build and
supplied an in-game screenshot of NCREC filling the screen. The workspace
contains the expected top menu, project list, camera preview placeholder,
inspector, transport, timeline and status bar.

**Confirmed:** rendering of the fullscreen app shell in Cyberpunk.

**Not confirmed:** F8 hide, active gameplay stability, Alt+F4 for this
specific rendering build, camera control, or native editor input.

**Expected limitation reported by owner:** the mouse is unavailable for
NCREC interaction because `PassiveFullscreen` intentionally keeps game input
ownership and suppresses mouse events in ImGui. This is **not yet a
functional UI**; no current configuration can make it clickable safely.

The next implementation must acquire and release the game's cursor like a
native editor, rather than using the old ASI WndProc interception alone.
CET's `InputSystemWin32Base::ForceCursor` is a research reference for the
required engine-owned cursor state; NCREC remains independent of CET.

**Safety note:** the versioned RED4ext SDK's `UniversalRelocFunc` may
terminate the game if an address hash cannot be resolved, so the input
adapter must fail closed with an independently checked resolver and must not
call unverified engine functions during bootstrap. Default remains
render-only until these gates have passed in-game.
