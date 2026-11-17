# RED4ext keyboard setup and safe input gate foundation

**Date:** 2026-11-17  
**Branch:** feature/safe-input-onboarding-gate  
**Mode:** Opt-in passive fullscreen on Cyberpunk 2077 2.50

## Code changes (NOT YET GAME-TESTED)

- A portable `EditorInputGate` requires a successful engine-provided
  acquisition before input can become active. It releases on focus loss,
  hide and shutdown. **No REDengine adapter is connected yet.**
- Unit tests cover denied acquisition, focus loss, repeated close,
  destructor cleanup and reserved key rejection.
- The preview shows keyboard-only setup if `[Hotkeys] ToggleVK` is
  absent. Supported: F6-F9, F11-F12, Insert, Home, End, PageUp,
  PageDown. CET F10 remains reserved.
- The selection is written to NCREC.ini without consuming messages,
  changing mouse clipping or hooking the game WndProc.

## Automated checks

- [ ] Windows DLL compilation (GitHub Actions)
- [ ] Portable native unit tests (GitHub Actions)

## In-game QA required for **this change only**

- [ ] Preview INI without saved ToggleVK: wizard appears automatically
- [ ] F11 selection + Enter saves; wizard closes
- [ ] F11 toggles preview, persists across restart
- [ ] Esc hides wizard and F8 reopens it before saving
- [ ] CET F10, Alt+F4 and normal game exit all work
- [ ] Game mouse and keyboard still work; UI remains render-only
- [ ] Focus loss, return and game menu changes remain stable

The prior 6/6 loader bootstrap test was already recorded elsewhere.
Do not interpret a successful compile as a real engine cursor-ownership
test. The game-aware cursor/input adapter is still deliberately disabled.
If unstable, remove NCREC.ini.
