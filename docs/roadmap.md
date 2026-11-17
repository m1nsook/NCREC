# Roadmap

## v0.1 — native renderer (experimental feature branch)

- [x] Portable C++20 camera path core and tests
- [x] Initial standalone Win32 ImGui bootstrap and console
- [x] Experimental DX12 in-game overlay backend and one-key toggle
- [x] First-run keybinding wizard, persisted keyboard selection and settings
- [x] Modern dark ImGui theme, Inter/JetBrains Mono resource embedding
- [ ] Real-game validation of onboarding, fonts and rebinding
- [ ] **Validate DX12 in-game overlay with actual Cyberpunk 2077**
- [ ] Test screenshots while using Snipping Tool / losing focus
- [ ] Validate input capture and interaction with CET, OBS and ReShade
- [ ] Validate resize, Alt+Tab, HDR, device reset and render thread handling

## v0.2 — camera path development

- [x] Portable camera rig math and fail-closed game-camera adapter contract
- [x] Virtual pose editor with X/Y/Z, FOV and timed keyframe capture/removal
- [x] JSON save/load of named shots and validation tests
- [ ] Version-aware native game camera read/override adapter and reliable restore
- [ ] Capture **real** game camera keyframes
- [ ] Replay paths in the **actual** game view

## v0.3 — editor + capture

- [ ] Timeline editing and camera curves
- [ ] Position/rotation/FOV control and tracking
- [ ] OBS WebSocket automation, then optional OBS dock/plugin

## RED4ext migration gates

- [x] RED4ext API-v1 bootstrap as separate native DLL target
- [x] Fullscreen workspace geometry from demo.html, portable and testable
- [x] Validate RED4ext host in Cyberpunk 2.50: DLL load/unload logs, menu/gameplay input and clean exit (user-tested 2026-11-17)
- [x] Give the passive renderer a configurable single-key toggle (F8 default), avoiding CET's F10
- [x] Keyboard-only first-launch RED4ext preview wizard: single-key selection, Enter to persist, Esc hide and F8 reopen (CI and game QA pending)
- [x] Reserved-key policy: CET F10, F4, modifiers and gameplay keys rejected; unit tests
- [x] Portable fail-closed input ownership gate with denied-acquisition/focus-loss/release tests (engine adapter NOT connected)
- [ ] Clickable wizard and settings when a verified game input adapter is available
- [ ] Game-aware cursor/input adapter and safe acquisition/release on Cyberpunk 2.50
- [x] Passive fullscreen DX12 workspace visibly renders over Cyberpunk 2.50 (user screenshot, 2026-11-17)
- [ ] **Interactive native mouse/cursor ownership**: request game cursor, route input to ImGui, restore game input on close/focus loss/unload
- [ ] Fullscreen editor interactive validation (clickable inspector, keyboard, F8 hide, CET coexistence)
- [ ] Reliable fullscreen DX12 UI replacing the floating editor, without nested scrollbars
- [ ] Version-aware active game camera read/override/restore
