#pragma once

#include <windows.h>

namespace ncrec::platform {

// Initializes native DXGI interception. The UI is rendered into the game's
// swapchain on Present, not through a separate top-level Win32 window.
DWORD runOverlay(HMODULE module);

} // namespace ncrec::platform
