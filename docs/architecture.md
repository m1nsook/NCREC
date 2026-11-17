# NCREC architecture (experimental DX12 renderer)

NCREC aims to provide a standalone camera path editor, independent of CET.

1. **Core:** portable camera keyframes and timestamped evaluation, timeline
   session/commands. Does not access game memory.
2. **Renderer:** native x64 ASI hooks DXGI factory/swapchain creation and
   draws Dear ImGui into the game's existing D3D12 framebuffer on Present.
   It uses the game's window for input, without a second Win32 window.
3. **Camera adapter (NOT IMPLEMENTED):** native version-aware camera
   capture/override/restore.
4. **Capture adapter (NOT IMPLEMENTED):** OBS WebSocket initially; eventual
   optional OBS Studio plugin.

## Why this branch exists

The previous prototype created a separate D3D11 topmost Win32 window
and deliberately hid on foreground loss. This prevented the editor from
remaining visibly integrated with a game screenshot when switching focus.
A correct in-game overlay instead renders as part of the game frame itself.
On this feature branch, the external-window backend was replaced by an
experimental DX12 injected renderer. The old source file is retained in git
history, not built.

The renderer:
- Draws Dear ImGui only while the selected single toggle key is enabled.
- Captures keyboard/mouse messages in the game's HWND while visible.
- Does not hide just because Windows loses focus.
- Uses the game DX12 swapchain, so the overlay can appear in final-frame
  screenshots, depending on when the screenshot tool captures the frame.
- Saves the chosen one-key hotkey in `NCREC.ini`.

## Critical compatibility questions

- Whether the ASI loader enters before Cyberpunk creates its DXGI factory.
- Whether native `Present`/resize interception works with current game,
  CET, ReShade, Steam, Discord, HDR and OBS.
- Whether game input also uses RawInput or another path needing focus gating.
- Whether DX12 resource transitions, swapchain recreation, render-thread
  safety and device loss are handled safely.

Do not call this renderer stable on the basis of a successful CI build.
No unsafe hard-coded game-camera offsets have been added.

## Roadmap

1. Verify game launch, UI toggle, click interaction, screenshot persistence.
2. Fix any DX12 hook/input conflicts and validate all render lifecycle cases.
3. Research and implement safe camera adapter (read/override/restore).
4. Add project serialization, editor timeline, OBS recording integration.
