#include "platform/red4ext/RenderProbe.hpp"

#include "core/Session.hpp"
#include "editor/FullscreenEditor.hpp"

#include <overlay.h>
#include <imgui.h>

#include <array>
#include <chrono>
#include <filesystem>
#include <string>

namespace ncrec::platform::red4ext {
namespace {
HMODULE g_module{};
bool g_started{};
bool g_styled{};
std::string g_toggleLabel{"F8"};
Session g_session;
editor::FullscreenEditorState g_ui;
std::chrono::steady_clock::time_point g_lastFrame{std::chrono::steady_clock::now()};

std::filesystem::path moduleFolder(HMODULE module) {
    std::array<wchar_t, 32768> path{};
    const DWORD length = GetModuleFileNameW(module, path.data(),
                                            static_cast<DWORD>(path.size()));
    if (length == 0 || length >= path.size()) return {};
    return std::filesystem::path(std::wstring(path.data(), length)).parent_path();
}

ImFont* loadFont(int resourceId, float pixels) {
    const HRSRC resource = FindResourceW(
        g_module, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
    if (!resource) return nullptr;
    HGLOBAL handle = LoadResource(g_module, resource);
    if (!handle) return nullptr;
    const void* data = LockResource(handle);
    const DWORD bytes = SizeofResource(g_module, resource);
    if (!data || bytes == 0) return nullptr;
    ImFontConfig fontConfig{};
    fontConfig.FontDataOwnedByAtlas = false;
    fontConfig.OversampleH = 2;
    fontConfig.OversampleV = 2;
    return ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
        const_cast<void*>(data), static_cast<int>(bytes), pixels, &fontConfig);
}

std::string keyLabel(UINT key) {
    if (key >= VK_F6 && key <= VK_F12)
        return "F" + std::to_string(key - VK_F1 + 1);
    switch (key) {
        case VK_INSERT: return "Insert";
        case VK_HOME: return "Home";
        case VK_END: return "End";
        case VK_PRIOR: return "PageUp";
        case VK_NEXT: return "PageDown";
        default: return "F8";
    }
}

void configureFonts() {
    // Loaded from resources embedded in this same RED4ext DLL.
    if (ImFont* body = loadFont(101, 14.0f))
        ImGui::GetIO().FontDefault = body;
    (void)loadFont(102, 13.0f);
}

void drawPassiveFrame(IDXGISwapChain3*, UINT, UINT) {
    if (!g_styled) {
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;
        style.ChildRounding = 0.0f;
        style.FrameRounding = 5.0f;
        style.FramePadding = ImVec2(11, 6);
        style.ItemSpacing = ImVec2(8, 7);
        style.WindowBorderSize = 0;
        style.ChildBorderSize = 1;
        style.Colors[ImGuiCol_Border] = ImVec4(.22f, .22f, .22f, 1);
        style.Colors[ImGuiCol_Button] = ImVec4(.19f, .19f, .19f, 1);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(.25f, .25f, .25f, 1);
        g_styled = true;
    }

    auto now = std::chrono::steady_clock::now();
    double delta = std::chrono::duration<double>(now - g_lastFrame).count();
    g_lastFrame = now;
    if (delta > 0.0 && delta < 0.25)
        g_session.advance(delta);

    editor::drawFullscreenEditor(g_session, g_ui);

    // Strong visible indication that this is NOT the interactive editor.
    const ImVec2 origin(18.0f, ImGui::GetIO().DisplaySize.y - 53.0f);
    ImGui::GetForegroundDrawList()->AddRectFilled(
        origin, ImVec2(origin.x + 570.0f, origin.y + 24.0f),
        IM_COL32(17, 17, 17, 220));
    ImGui::GetForegroundDrawList()->AddText(
        ImVec2(origin.x + 8.0f, origin.y + 4.0f),
        IM_COL32(255, 221, 34, 255),
        (std::string("RENDER-ONLY PREVIEW - ") + g_toggleLabel +
         " TO HIDE - GAME INPUT UNCHANGED").c_str());

    if (g_ui.closeRequested) {
        g_ui.closeRequested = false;
        overlay::set_visible(false);
    }
}

} // namespace

bool startPassiveFullscreen(HMODULE module, UINT toggleKey) {
    if (g_started || !module) return false;
    const std::filesystem::path folder = moduleFolder(module);
    if (folder.empty()) return false;

    g_module = module;
    g_toggleLabel = keyLabel(toggleKey);
    g_ui.status = "PASSIVE RENDER TEST - " + g_toggleLabel +
                  " toggles. Input remains in the game.";
    g_session.setProjectDirectory(folder / L"projects");
    g_lastFrame = std::chrono::steady_clock::now();
    g_styled = false;

    overlay::set_passive_mode(true);
    overlay::set_toggle_key(toggleKey);
    overlay::set_visible(false);
    overlay::set_font_config_callback(&configureFonts);
    overlay::set_render_callback(&drawPassiveFrame);
    overlay::init();
    g_started = true;
    return true;
}

void stopPassiveFullscreen() {
    if (!g_started) return;
    overlay::set_visible(false);
    overlay::deinit();
    g_started = false;
}

} // namespace ncrec::platform::red4ext
