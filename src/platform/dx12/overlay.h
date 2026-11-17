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
void set_toggle_key(UINT vk_code = VK_OEM_MINUS);
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
