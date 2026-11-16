#include "platform/OverlayWindow.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Keep DllMain minimal. All Win32, graphics, and ImGui setup happens later
// on the worker thread, after the loader lock has been released.
namespace {

DWORD WINAPI startPlugin(LPVOID parameter) {
    return ncrec::platform::runOverlay(static_cast<HMODULE>(parameter));
}

} // namespace

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID /*reserved*/) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        HANDLE worker = CreateThread(nullptr, 0, startPlugin, instance, 0, nullptr);
        if (worker != nullptr) {
            CloseHandle(worker);
        } else {
            OutputDebugStringA("NCREC: failed to create bootstrap thread\n");
        }
    }
    // ASI hot-unloading is not supported by this bootstrap.
    return TRUE;
}
