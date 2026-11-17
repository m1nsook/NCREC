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

## v0.2 — usable camera paths

- [ ] Version-aware native camera adapter and reliable restore
- [ ] Capture real camera keyframes
- [ ] Replay paths in the actual game view
- [ ] Save/load camera paths and editor project files

## v0.3 — editor + capture

- [ ] Timeline editing and camera curves
- [ ] Position/rotation/FOV control and tracking
- [ ] OBS WebSocket automation, then optional OBS dock/plugin
