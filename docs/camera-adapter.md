# NCREC camera adapter design (v0.2 preparation)

## Current state

The `feature/camera-controller` branch has a working **offline/virtual**
camera rig and keyframe editor. **No live Cyberpunk view is modified**.
The engine boundary is represented by `IGameCameraAdapter` and
`CameraTransport` in `src/core/CameraTransport.hpp`.

- `CameraRig`: right/forward/up motion with world-Z-up, camera-local-Y-forward
  coordinates; quaternion yaw/pitch/roll, finite input validation, diagonal
  movement normalization and bounded step duration.
- `Session`: preview pose, indexed keyframe deletion, per-time capture,
  playback sampling and commands for pose edits.
- `ProjectIO`: versioned `*.ncrec.json` files with atomically replaced
  saves, format/version checks, bounded parsing, and CameraPath validation.
- `UnavailableGameCamera`: the only current game adapter. It intentionally
  refuses all read/write operations, so the editor cannot accidentally
  interfere with gameplay.

## Required engine integration contract

A supported game adapter must:

1. Confirm the Cyberpunk game version and appropriate camera mode.
2. Resolve and read the **actual active rendered camera**, not a component
   that is inactive while the Photo Mode drone or another cinematic camera
   owns the view.
3. Preserve all prior camera state when acquiring control: transform,
   FOV and other game camera ownership flags. Do not override unsupported
   or unknown transitions.
4. Apply a camera sample with safe thread/lifecycle timing after camera
   update but before the game renders its view.
5. Restore ownership, previous transforms and flags when disabled,
   loading a save, switching cameras, cutscene changes or unexpected
   adapter failure.
6. Fail closed if the game updates and any camera functions/symbols cannot
   be resolved. Do not guess memory offsets based on a prior version.

`CameraTransport::start()` rejects unsupported or unreadable camera
states. `CameraTransport::apply()` releases ownership after a failed
backend write. Unit tests use a fake adapter to verify the contract.

## REDengine research notes

The public panorama-taker project experimentally observed that
`gameCameraSystem:GetActiveCameraWorldTransform()` follows the real
Photo Mode camera, but the CET RTTI-exposed camera system does not
provide an absolute writable Photo Mode transform setter.

Reference:
https://github.com/Malgalad/panorama-taker/blob/master/docs/progress.md

Likewise, CET's cursor handling uses a separate game-specific
`InputSystemWin32Base::ForceCursor` integration rather than only
Win32 cursor clipping. This is relevant to NCREC's eventual native
input/camera adapter, not a reason to rely on CET's overlay.
Reference:
https://github.com/maximegmd/CyberEngineTweaks/blob/master/src/d3d12/D3D12.cpp

These observations are **research leads**, not proof of a stable
Cyberpunk 2.5 camera-write API. Native RED4ext RTTI and camera lifecycle
must be studied and tested in-game before connecting `CameraTransport`
to an actual engine backend.

## Coordinate convention

Core stores camera poses in NCREC-local coordinates for testability:
world up is +Z, forward is +Y, right is +X; the rotation quaternion is
`(x,y,z,w)`. A game adapter is responsible for converting transforms to
and from the exact REDengine coordinate convention without silently
swapping axes or mixing world/local rotations.

## Project format

Named files live at `bin/x64/plugins/NCREC/projects/<name>.ncrec.json`.
Names are limited to 1–64 ASCII letters, digits, underscore or hyphen.
The JSON format has an exact marker `NCREC.CameraPath`, integer version 1,
position interpolation and a timestamped keyframe list. Invalid files do
not replace the user's currently edited path.

Example:
```json
{
  "format": "NCREC.CameraPath",
  "version": 1,
  "positionInterpolation": "cubic_hermite",
  "keyframes": [
    {
      "time": 0.0,
      "position": [0, 0, 0],
      "rotation": [0, 0, 0, 1],
      "fov": 60.0
    }
  ]
}
```

## Next gated work

- Instrument the active camera read path after a saved game finishes loading,
  with game/version guards and clean failure reporting.
- Validate transform/FOV consistency during gameplay, vehicle and Photo Mode
  camera ownership changes.
- Prototype *reversible* camera control only when an actual supported camera
  setter or override hook is found. No arbitrary memory patch writes.
- Connect successful native read/override/restore to the existing
  `CameraRig`, `Session` keyframe capture and timeline playback.
