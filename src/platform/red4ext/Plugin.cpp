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

#include <atomic>
#include <cstdint>

namespace {
std::atomic_bool g_loaded{false};

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
        case RED4ext::v1::EMainReason::Load:
            if (g_loaded.exchange(true)) return true;
            logInfo(handle, sdk, "NCREC RED4ext host loaded (bootstrap only).");
            logInfo(handle, sdk,
                    "NCREC's fullscreen renderer, cursor interception and camera hooks "
                    "are intentionally disabled pending game-version validation.");
            return true;

        case RED4ext::v1::EMainReason::Unload:
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
    info->version = RED4EXT_V1_SEMVER(0, 2, 0);
    // API 1.0.0 defines the latest known game runtime as 2.31, while
    // development is being tested on Cyberpunk 2.5. This *loader-only*
    // bootstrap does not access game memory; version-specific features
    // MUST be blocked until a validated runtime gate is added.
    info->runtime = RED4EXT_V1_RUNTIME_VERSION_INDEPENDENT;
    info->sdk = RED4EXT_V1_SDK_VERSION_CURRENT;
}

RED4EXT_C_EXPORT std::uint32_t RED4EXT_CALL Supports() {
    return RED4EXT_API_VERSION_1;
}
