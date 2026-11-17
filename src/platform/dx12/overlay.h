#pragma once
#include <Windows.h>
#include <dxgi1_4.h>

namespace overlay {

using render_callback_t = void(*)(IDXGISwapChain3*, UINT, UINT);
using font_config_callback_t = void(*)();

void init();
void deinit();
void set_render_callback(render_callback_t render_callback);
void set_font_config_callback(font_config_callback_t callback);
// Must be set before init. Render-only preview never acquires or swallows
// game input; the user-configured key toggles visibility while focused.
void set_passive_mode(bool passive);
void set_toggle_key(UINT vk_code = VK_OEM_MINUS);
// Prevent the passive F8 toggle from dismissing a keyboard-only setup step.
// This never acquires input or modifies the game's window procedure.
void set_toggle_suspended(bool suspended);
// Non-blocking one-key capture for the first-run wizard and Settings.
// Return 0 while waiting; Escape and reserved keys are reported to UI.
void begin_key_capture();
void end_key_capture();
UINT take_captured_key();
void set_visible(bool visible);
bool is_visible();
void enable_logging(const char* path = "dx12_overlay_log.txt");
void disable_logging();

}
