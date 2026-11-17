# NCREC UI design specification (v0.1)

## Principles
- The editor renders into Cyberpunk's existing DX12 frame, not in CET and not in a second desktop window.
- Prioritize cinematic tasks over debugging controls. "Timeline", "Console", and "Settings" are separate views.
- Every mandatory action must work with **one hand**: clickable buttons and single-key shortcuts.
- Low visual noise, generous spacing, accessible text sizes, dark graphite backgrounds and the NCREC yellow accent.
- **Inter** is the application font. **JetBrains Mono** is used for console output, camera coordinates and timecodes.

## First launch
1. On startup, look for a valid \`[Hotkeys] ToggleVK\` in NCREC.ini.
2. If absent, migrate an old \`[Hotkeys] Toggle\` named key when possible.
3. If neither exists, automatically show a centered first-run setup window in the game frame.
4. Automatically listen for one keyboard key. Reject reserved modifier/system keys; warn for common gameplay bindings.
5. Show selected key, allow "Choose again" or "Save and finish".
6. Save the numeric Windows virtual-key code to \`NCREC.ini\`, then close the setup.
7. On subsequent launches, keep the editor hidden until the saved key is pressed.
8. The key can be changed later in Settings with the same capture flow.

No default bind is imposed on a fresh install. No Ctrl/Shift/Fn combination is required.

## Layout
- **Timeline:** transport controls, timestamp scrubbing, readout of position/FOV and preview status.
- **Console:** command input, history/log text, monospaced output.
- **Settings:** Controls (one-key binding) and Appearance (UI scale). Capture/OBS settings come later.

## Appearance tokens
- Background: near-black graphite
- Panels: slightly lighter graphite, subtle low-contrast border
- Accent: NCREC gold \`#FFCC17\` / related hover states
- Text: off-white; secondary text: muted grey
- Windows: 16 px corner radius
- Controls: 8 px corner radius, generous click targets
- Typography: Inter regular 17 px, headings 23 px; JetBrains Mono 15 px
- UI scale: 0.90–1.75, persisted as a percentage in \`[Appearance] UIScalePercent\`.

## Font Awesome 6.4.0 icons (RED4ext only)

The native RED4ext DLL now embeds the demo's Font Awesome Free 6.4.0
solid icon font at compile time and merges a small set of glyphs into Inter.
Menu actions, project panel, viewport tools, transport and inspector tabs
use icon labels with a text fallback. This is a visual-only change:
cursor/input ownership remains deliberately disabled.
License: `licenses/FontAwesome-Free-LICENSE.txt`.

## Font packaging
Fonts are downloaded by CMake from **immutable upstream commits**, then embedded
as Windows \`RCDATA\` resources into \`NCREC.asi\`. Users do not need Windows
font installation or separate font files. Source licenses are in
\`licenses/Inter-OFL.txt\` and \`licenses/JetBrainsMono-OFL.txt\`.

## Limitations / verification
- Verify first-run setup with no NCREC.ini and migration with an existing ini.
- Verify Save actually persists, plus repeated launches and key rebinding.
- Verify screenshot readability at 1080p, 1440p, 4K, UI scaling, different DPI.
- Verify input capture with CET/ReShade and avoid input leakage to gameplay.
- Font scaling above 1.0 is a prototype; higher-resolution font atlas variants may be needed for perfectly crisp 4K scaling.
- The current camera path is a math preview; camera control and OBS integration are not implemented.

### Input-safety requirements

- Windows system shortcuts, especially Alt+F4, must never be swallowed by NCREC.
- Escape always hides the overlay, including during a first-run key capture.
- Until first-run setup is completed, F10 is an emergency recovery toggle.
- Enter saves a captured first-run key without needing the mouse.
- The experimental cursor implementation uses ClipCursor release and a
  relative RawInput-to-ImGui fallback. This is **not the same as CET's engine-level
  ForceCursor state**. A version-safe game cursor adapter is planned once
  the native engine integration can be independently verified.
