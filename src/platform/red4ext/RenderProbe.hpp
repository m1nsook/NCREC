#pragma once

#include <Windows.h>

namespace ncrec::platform::red4ext {

// Opt-in rendering-only experiment. Never takes input or controls game state.
// Must be called from RED4ext's load/unload callbacks, not DllMain.
[[nodiscard]] bool startPassiveFullscreen(HMODULE module);
void stopPassiveFullscreen();

} // namespace ncrec::platform::red4ext
