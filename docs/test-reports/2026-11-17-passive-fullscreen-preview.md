# RED4ext fullscreen passive preview — visual feedback

- **Date:** 2026-11-17
- **Game:** Cyberpunk 2077 v2.50
- **Mode:** NCREC RED4ext `PassiveFullscreen=1`, F8 by default
- **Evidence:** owner-supplied screenshot in development conversation

## Confirmed

- [x] Fullscreen native ImGui shell appears over Cyberpunk
- [x] Top menu, project/shots sidebar, empty preview, right inspector,
      bottom timeline and status bar are visible
- [x] Render-only indicator visible; game camera remains disconnected

## Expected limitations / not yet tested

- [ ] Cursor control and clickable widgets **not available in passive mode**
- [ ] F8 hide and game keyboard/mouse coexistence in the same test session
- [ ] Stable during active gameplay and Alt+Tab
- [ ] Alt+F4/normal game exit with rendering preview enabled
- [ ] Real game camera read/override

The user reported the mouse could not control NCREC. This matches the code:
`src/platform/dx12/overlay.cpp` avoids subclassing or capturing input in
passive mode and explicitly discards ImGui mouse movement and buttons.

## Next implementation gate

Implement a **version-aware engine-owned cursor/input adapter**, with
acquire → interactive event routing → release transitions and fail-closed
fallback to passive mode. It must retain F8 emergency hide, CET F10
compatibility and Windows system shortcuts. Do not re-enable the old ASI
message-hook trap alone.

Research reference: https://github.com/maximegmd/CyberEngineTweaks/blob/master/src/d3d12/D3D12.cpp
