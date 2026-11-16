# NCREC architecture (prototype)

NCREC is an open-source cinematic camera path editor for Cyberpunk 2077,
inspired by HLAE's mirv_campath. The eventual application has four layers:

1. **Core (platform independent):** timestamped camera keyframes, interpolation,
   playback state and a small command system. No game memory access.
2. **Native host (Windows ASI):** own UI and console, no CET overlay.
3. **Game camera adapter (NOT IMPLEMENTED):** verified, version-aware camera
   read/write, safe restore, gameplay/cutscene lifecycle handling.
4. **Capture adapters (NOT IMPLEMENTED):** OBS WebSocket initially; later
   optional OBS frontend plugin.

## Bootstrap UI strategy

The early native prototype uses a **separate Win32/D3D11 window with its own
Dear ImGui context** from inside the ASI plugin. It is NOT a DirectX 12
Present hook and does not render into Cyberpunk's swapchain. This minimizes
initial render-hook conflicts but is only intended for windowed/borderless
gameplay. It must not be advertised as a full exclusive-fullscreen overlay.
The window is hidden unless toggled with Ctrl+Shift+F10; Ctrl+F10 opens
the same window with the console focused.

The UI is deliberately independent of CET. Current playback only evaluates
sample data: **it does not manipulate the game's camera**.

## Camera timeline semantics

- Keyframes have nonnegative unique times in seconds.
- Invalid camera data is rejected; rotations are normalized.
- Position: time-aware cubic Hermite interpolation or linear fallback.
- Orientation: shortest-arc quaternion SLERP; FOV: linear.
- Evaluation is based on an absolute timestamp, not incremental motion.
- Endpoint times clamp; an empty path has no camera to evaluate.

## Next acceptance gates

1. Verify a signed/reproducible x64 ASI build loads on current game versions.
2. Verify UI visibility, input focus and coexistence with CET, ReShade and OBS
   in borderless/windowed game modes.
3. Research and implement a game camera adapter **without hard-coded,
   unverified memory offsets**; test safe restoration before keyframe capture.
4. Add editing, path serialization, recording control and OBS integration.

## Constraints

Avoid blocking the render/game thread with filesystem I/O or network work.
Keep per-game-version integration isolated. Keep the core testable without
Cyberpunk installed. ASI/loader use must not disable game or platform safety.
