# RED4ext bootstrap — in-game verification

- **Date:** 2026-11-17
- **Tested by:** project owner
- **Game:** Cyberpunk 2077 2.50
- **RED4ext:** 1.31.1
- **NCREC:** 0.2.0 native RED4ext DLL (loader-only)
- **Result:** PASS, six of six manual checks

## Manual checklist (confirmed by owner)

- [x] Remove previous NCREC.asi
- [x] Extract development ZIP into Cyberpunk 2077 folder
- [x] Launch game and reach main menu
- [x] Find NCREC load message in RED4ext logs
- [x] Verify mouse and keyboard input work
- [x] Verify Alt+F4 and normal game exit work

## Log evidence

Owner-provided logs dated 2026-11-17:

- RED4ext startup reports product version **2.50**, RED4ext **v1.31.1**.
- RED4ext loads NCREC plugin **v0.2.0**.
- NCREC logs `NCREC RED4ext host loaded (bootstrap only).`.
- NCREC logs that fullscreen renderer and camera/input hooks are disabled.
- NCREC logs `NCREC RED4ext host unloaded.` during clean RED4ext shutdown.

The game was exited normally. These logs and the checklist together
support the bootstrap PASS; the logs alone cannot prove input behavior.

## Explicitly not yet tested or enabled

- Native fullscreen NCREC editor during active gameplay
- Engine-owned cursor acquisition and release
- DX12 swapchain and rendering lifecycle with third-party overlays
- Game camera control, recording and playback

## Developer rule

**Do not ask the owner to repeat these six bootstrap checks unless a new
DLL version or host integration change materially affects startup, input or
shutdown.** Test milestones must be tracked here and in `docs/roadmap.md`,
rather than relying on conversational memory.
