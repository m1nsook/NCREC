#include "platform/GameOverlay.hpp"

#include <windows.h>

// ASI bootstrap. Loader-lock operations are deliberately kept minimal;
// the DXGI hooks are installed from a worker thread.
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
            OutputDebugStringA("NCREC: failed to start bootstrap thread\n");
        }
    }
    // Do not unhook on loader-lock detach. Hot unloading is unsupported.
    return TRUE;
}
