#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ncrec::platform {

// Returns when the overlay thread exits. No game memory is modified.
DWORD runOverlay(HMODULE module);

} // namespace ncrec::platform
