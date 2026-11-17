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
- [ ] Validate RED4ext host in Cyberpunk 2.5 (logs and clean exit)
- [ ] Safe game-aware input/cursor ownership and editor activation lifecycle
- [ ] Reliable fullscreen DX12 UI replacing the floating editor, without nested scrollbars
- [ ] Version-aware active game camera read/override/restore
