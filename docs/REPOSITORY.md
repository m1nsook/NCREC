# Repository workflow and status

## Where to find current code

**Always start at [main](https://github.com/m1nsook/NCREC/tree/main).**
This is the authoritative development snapshot, not a stable game release.
Completed work moves to `main` after build/tests; no indefinite nested draft PRs.

- `src/core/` — camera math, camera-path serialization and editor state
- `src/editor/` — fullscreen geometry and compiled ImGui app shell (currently not activated by DLL)
- `src/platform/red4ext/` — new native DLL host
- `src/platform/GameOverlay.cpp` and `src/platform/dx12/` — **legacy ASI experiment**; do not install for normal play
- `design/demo.html` — owner-provided full-screen design target
- `docs/red4ext-migration.md` — compatibility/validation gates

## Branching rules

1. `main`: canonical checked-in source, can contain **disabled experimental code** but is not marketed as release-ready.
2. `feature/*`: short-lived work; a pull request targets `main` directly.
3. CI must pass before merging; runtime-dependent work also requires actual in-game validation before switching it on by default.
4. Prefer early merges for isolated and disabled code. Keep draft PRs only for genuine blockers; don't stack them.
5. Releases/tags are reserved for a validated user-installable build.
6. Legacy ASI is **OFF by default** in CMake. RED4ext loader-only DLL is default on Windows.

## Current runtime state

The RED4ext target is a loader-only bootstrap: no ImGui/renderer, no game cursor ownership, no camera writes. The legacy ASI prototype previously rendered ImGui but was unstable during gameplay. It is **not** a replacement for the RED4ext editor.

The goal is the owner-supplied fullscreen `demo.html` layout: application-like top bar, project sidebar, center preview, inspector, timeline, status bar, and minimal scrolling. No unsafe rendering/input hooks will be enabled without runtime validation.
