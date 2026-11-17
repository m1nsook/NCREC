# Native RED4ext Font Awesome icon integration

**Date:** 2026-11-17  
**Scope:** Experimental passive fullscreen (no game input/camera changes)

## Code changes

- [x] Pin Font Awesome Free 6.4.0 solid icon font.
- [x] Embed font in native NCREC.dll as Win32 RCDATA.
- [x] Merge required icon glyphs into the Inter font atlas.
- [x] Replace viewport S/H/O/+/# placeholders with icons.
- [x] Add icons to menus, project actions, playback and inspector tabs.
- [x] Include the original SIL OFL license in the development ZIP.
- [ ] Windows DLL compilation and native tests (GitHub Actions).
- [ ] Confirm visuals with screenshot in Cyberpunk 2.50.

## In-game visual QA (not a cursor input test)

With the updated DLL and existing opt-in preview configuration, open
the full-screen shell at the main or pause menu. Verify actual icons
rather than letter placeholders or missing-glyph squares in the
viewport toolbar, top menu, project panel, transport and inspector.
Compare layout and icon alignment with `design/demo.html`.

**The game's mouse remains unaffected and cannot click NCREC controls.**
That is an already-known limitation. Do not repeat the previously
completed RED4ext loader bootstrap checklist unless startup regresses.
Game-owned cursor/input acquisition remains future work.
