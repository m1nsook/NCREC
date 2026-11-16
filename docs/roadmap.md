# Roadmap

## v0.1 — native bootstrap (in progress)

- [x] Independent, buildable C++20 camera path core
- [x] Unit tests for interpolation and timeline behavior
- [x] Own ASI plugin target and independent UI/console skeleton
- [ ] Validate the plugin and window inside Cyberpunk 2077
- [ ] Verify input behavior and compatibility with common overlays

## v0.2 — usable in-game camera paths

- [ ] Version-aware camera adapter with safe enable/disable
- [ ] Keyframe capture from the actual game camera
- [ ] Path playback affecting the game camera
- [ ] Save/load project files
- [ ] Freecam movement and editor hotkeys

## v0.3 — cinematic editor

- [ ] Timeline editing, scrubbing, drag-and-drop keyframes
- [ ] Position/rotation/FOV curves and easing
- [ ] Camera path visualization
- [ ] OBS WebSocket recording automation

## Later

- [ ] Camera look-at targets, subject tracking, lens controls
- [ ] External shot-control API, optional OBS Studio dock/plugin
- [ ] Frame-step/capture experiments and reproducible scene tooling
