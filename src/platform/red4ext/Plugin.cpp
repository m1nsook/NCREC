// RED4ext plugin bootstrap. This target is deliberately separate from
// NCREC.asi: RED4ext owns this DLL's Load/Unload lifecycle.
#include <RED4ext/Common.hpp>
#include <RED4ext/Api/ApiVersion.hpp>
#include <RED4ext/Api/v1/EMainReason.hpp>
#include <RED4ext/Api/v1/PluginHandle.hpp>
#include <RED4ext/Api/v1/PluginInfo.hpp>
#include <RED4ext/Api/v1/Runtime.hpp>
#include <RED4ext/Api/v1/Sdk.hpp>
#include <RED4ext/Api/v1/Version.hpp>

#include "platform/red4ext/RenderProbe.hpp"

#include <atomic>
#include <cstdint>
#include <array>
#include <filesystem>
#include <string>

namespace {
std::atomic_bool g_loaded{false};
bool g_probeStarted{};

bool passiveProbeRequested(const RED4ext::v1::Sdk* sdk) noexcept {
    if (!sdk || !sdk->runtime || sdk->runtime->major != 2 ||
        sdk->runtime->minor != 5 || sdk->runtime->patch != 0) {
        return false; // Tested only on Cyberpunk's reported product v2.50.
    }
    const HMODULE module = GetModuleHandleW(L"NCREC.dll");
    if (!module) return false;

    std::array<wchar_t, 32768> modulePath{};
    const DWORD count = GetModuleFileNameW(
        module, modulePath.data(), static_cast<DWORD>(modulePath.size()));
    if (!count || count >= modulePath.size()) return false;

    const auto ini = std::filesystem::path(
        std::wstring(modulePath.data(), count)).parent_path() / L"NCREC.ini";
    return GetPrivateProfileIntW(
        L"Experimental", L"PassiveFullscreen", 0, ini.c_str()) == 1;
}

void logInfo(RED4ext::v1::PluginHandle handle, const RED4ext::v1::Sdk* sdk,
             const char* message) noexcept {
    if (sdk && sdk->logger && sdk->logger->Info)
        sdk->logger->Info(handle, message);
}

} // namespace

// This is the one and only entry point for the RED4ext build. Unlike the ASI
// prototype, we do not create a thread or install DXGI hooks inside DllMain.
RED4EXT_C_EXPORT bool RED4EXT_CALL Main(
    RED4ext::v1::PluginHandle handle,
    RED4ext::v1::EMainReason reason,
    const RED4ext::v1::Sdk* sdk) {

    switch (reason) {
        case RED4ext::v1::EMainReason::Load: {
            // Fail closed when the legacy ASI has already been loaded.
            // This protects against installing both NCREC variants at once.
            if (GetModuleHandleW(L"NCREC.asi") != nullptr) {
                if (sdk && sdk->logger && sdk->logger->Error) {
                    sdk->logger->Error(handle,
                        "NCREC: legacy NCREC.asi is already loaded. "
                        "Remove the ASI before using the RED4ext DLL.");
                }
                return false;
            }
            if (g_loaded.exchange(true)) return true;
            logInfo(handle, sdk, "NCREC RED4ext host loaded.");
            const HMODULE module = GetModuleHandleW(L"NCREC.dll");
            if (passiveProbeRequested(sdk) && module) {
                g_probeStarted = ncrec::platform::red4ext::startPassiveFullscreen(module);
                if (g_probeStarted) {
                    logInfo(handle, sdk,
                        "EXPERIMENTAL passive fullscreen renderer initialized; "
                        "press F10 in-game. No input ownership or camera hooks.");
                } else if (sdk && sdk->logger && sdk->logger->Error) {
                    sdk->logger->Error(handle, "NCREC passive preview failed to initialize.");
                }
            } else {
                logInfo(handle, sdk,
                    "NCREC fullscreen renderer is disabled by default; "
                    "the verified RED4ext bootstrap remains active.");
            }
            return true;
        }

        case RED4ext::v1::EMainReason::Unload:
            if (g_probeStarted) {
                ncrec::platform::red4ext::stopPassiveFullscreen();
                g_probeStarted = false;
            }
            if (g_loaded.exchange(false)) {
                logInfo(handle, sdk, "NCREC RED4ext host unloaded.");
            }
            return true;
    }
    return false;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::v1::PluginInfo* info) {
    if (!info) return;
    info->name = L"NCREC";
    info->author = L"m1nsook";
    info->version = RED4EXT_V1_SEMVER(0, 3, 0);
    // Version independent for safe default loading. Experimental
    // render-only preview is separately gated to Cyberpunk 2.50 at runtime.
    // No REDengine memory or camera hooks are enabled.
    info->runtime = RED4EXT_V1_RUNTIME_VERSION_INDEPENDENT;
    info->sdk = RED4EXT_V1_SDK_VERSION_CURRENT;
}

RED4EXT_C_EXPORT std::uint32_t RED4EXT_CALL Supports() {
    return RED4EXT_API_VERSION_1;
}
