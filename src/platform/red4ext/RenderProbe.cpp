#include "platform/red4ext/RenderProbe.hpp"

#include "core/Session.hpp"
#include "core/ToggleKeyPolicy.hpp"
#include "editor/FullscreenEditor.hpp"

#include <overlay.h>
#include <imgui.h>

#include <array>
#include <cwchar>
#include <cstdint>
#include <chrono>
#include <filesystem>
#include <string>

namespace ncrec::platform::red4ext {
namespace {
HMODULE g_module{};
bool g_started{};
bool g_styled{};
std::string g_toggleLabel{"F8"};
std::filesystem::path g_iniPath;
bool g_firstRunWizard{};
std::uint32_t g_candidateKey{};
std::string g_wizardFeedback;
std::array<bool, input::kToggleKeys.size()> g_wasDown{};
bool g_wasEnterDown{};
bool g_wasEscapeDown{};
bool g_keysPrimed{};
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

// A passive preview has no engine-owned cursor or WndProc interceptor.
// Only poll approved single keys while the game is in the foreground.
// This cannot make the editor's mouse widgets interactive.
bool keyDown(int virtualKey) noexcept {
    return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
}

UINT savedKeyFromIni() {
    wchar_t value[64]{};
    GetPrivateProfileStringW(L"Hotkeys", L"ToggleVK", L"", value,
        static_cast<DWORD>(std::size(value)), g_iniPath.c_str());
    if (!value[0]) return 0;
    wchar_t* end = nullptr;
    const auto parsed = std::wcstoul(value, &end, 10);
    return (end != value && *end == 0 && input::allowedToggleKey(parsed))
        ? static_cast<UINT>(parsed) : 0;
}

bool saveFirstRunKey() {
    const std::wstring vk = std::to_wstring(g_candidateKey);
    const std::string_view label = input::toggleKeyName(g_candidateKey);
    if (label.empty()) return false;
    const std::wstring named(label.begin(), label.end());
    if (!WritePrivateProfileStringW(
            L"Hotkeys", L"ToggleVK", vk.c_str(), g_iniPath.c_str()) ||
        !WritePrivateProfileStringW(
            L"Experimental", L"ToggleKey", named.c_str(), g_iniPath.c_str())) {
        g_wizardFeedback = "Unable to save NCREC.ini. Check folder permissions.";
        return false;
    }
    g_toggleLabel = std::string(label);
    g_ui.status = "Saved " + g_toggleLabel +
        ". Render-only preview: game input unchanged.";
    g_firstRunWizard = false;
    g_keysPrimed = false;
    overlay::set_toggle_key(g_candidateKey);
    overlay::set_toggle_suspended(false);
    overlay::set_visible(false);
    return true;
}

void pollFirstRunKeyboard(HWND gameWindow) {
    if (!g_firstRunWizard || !gameWindow ||
        GetForegroundWindow() != gameWindow) {
        g_keysPrimed = false;
        return;
    }

    std::array<bool, input::kToggleKeys.size()> pressed{};
    for (std::size_t i = 0; i < pressed.size(); ++i)
        pressed[i] = keyDown(static_cast<int>(input::kToggleKeys[i].vk));
    const bool enter = keyDown(VK_RETURN);
    const bool escape = keyDown(VK_ESCAPE);

    if (!g_keysPrimed) {
        g_wasDown = pressed;
        g_wasEnterDown = enter;
        g_wasEscapeDown = escape;
        g_keysPrimed = true;
        return; // Never capture a hotkey that was already held at activation.
    }

    if (escape && !g_wasEscapeDown) {
        overlay::set_visible(false);
        overlay::set_toggle_suspended(false);
        g_wizardFeedback = "Setup hidden. F8 reopens it.";
        g_keysPrimed = false;
        return;
    }

    // Do not interpret keyboard combinations or system shortcuts as binds.
    const bool modifiers = keyDown(VK_SHIFT) || keyDown(VK_CONTROL) ||
        keyDown(VK_MENU) || keyDown(VK_LWIN) || keyDown(VK_RWIN);
    if (!modifiers) {
        for (std::size_t i = 0; i < pressed.size(); ++i) {
            if (pressed[i] && !g_wasDown[i]) {
                g_candidateKey = input::kToggleKeys[i].vk;
                g_wizardFeedback.clear();
                break;
            }
        }
        if (enter && !g_wasEnterDown && g_candidateKey)
            (void)saveFirstRunKey();
    }
    g_wasDown = pressed;
    g_wasEnterDown = enter;
    g_wasEscapeDown = escape;
}

void drawFirstRunWizard() {
    const ImVec2 size = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos({size.x * .5f, size.y * .5f},
                             ImGuiCond_Always, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({520, 274}, ImGuiCond_Always);
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(.085f, .085f, .085f, 1));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(.45f, .45f, .25f, 1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::Begin("NCREC First Launch##red4ext", nullptr, flags);
    ImGui::TextColored({.99f, .93f, .04f, 1}, "NCREC  /  FIRST LAUNCH");
    ImGui::Separator();
    ImGui::Dummy({0, 8});
    ImGui::TextUnformatted("Choose a single key for your editor.");
    ImGui::TextWrapped("Press F6-F9, F11-F12, Insert, Home, End, PageUp "
                       "or PageDown. F10 is reserved for CET.");
    ImGui::Dummy({0, 8});
    if (g_candidateKey) {
        const auto label = input::toggleKeyName(g_candidateKey);
        ImGui::TextColored({.99f, .93f, .04f, 1},
            "Selected: %.*s", static_cast<int>(label.size()), label.data());
        ImGui::TextUnformatted("Press ENTER to save and close setup.");
    } else {
        ImGui::TextDisabled("Listening for an allowed single key...");
    }
    if (!g_wizardFeedback.empty())
        ImGui::TextWrapped("%s", g_wizardFeedback.c_str());
    ImGui::Dummy({0, 10});
    ImGui::Separator();
    ImGui::TextDisabled("ESC hides setup; F8 reopens it until saved.");
    ImGui::TextDisabled("Keyboard-only setup. Game input is NOT captured.");
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
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
    // Merge Font Awesome with Inter, NOT with the later JetBrains Mono font.
    ImGuiIO& io = ImGui::GetIO();
    ImFont* body = loadFont(101, 14.0f);
    if (!body) body = io.Fonts->AddFontDefault();
    io.FontDefault = body;
    (void)loadFont(102, 13.0f);

    const HRSRC icon = FindResourceW(g_module, MAKEINTRESOURCEW(103), RT_RCDATA);
    if (!icon) return; // Text-only fallback on a missing icon resource.
    HGLOBAL handle = LoadResource(g_module, icon);
    const void* data = handle ? LockResource(handle) : nullptr;
    const DWORD bytes = SizeofResource(g_module, icon);
    if (!data || !bytes) return;

    // Small exact glyph set instead of rasterizing the entire icon font.
    static const ImWchar ranges[] = { 0xe29c, 0xe29c, 0xe4bb, 0xe4bb, 0xf008, 0xf008, 0xf00d, 0xf00d, 0xf00e, 0xf00e, 0xf013, 0xf013, 0xf030, 0xf030, 0xf03d, 0xf03d, 0xf044, 0xf044, 0xf048, 0xf048, 0xf04b, 0xf04b, 0xf04c, 0xf04c, 0xf051, 0xf051, 0xf055, 0xf055, 0xf059, 0xf059, 0xf06e, 0xf06e, 0xf07c, 0xf07c, 0xf0c7, 0xf0c7, 0xf0db, 0xf0db, 0xf120, 0xf120, 0xf15b, 0xf15b, 0xf245, 0xf245, 0xf256, 0xf256, 0xf2ed, 0xf2ed, 0xf390, 0xf390, 0xf84c, 0xf84c, 0 };
    ImFontConfig config{};
    config.MergeMode = true;
    config.DstFont = body;
    config.FontDataOwnedByAtlas = false;
    config.PixelSnapH = true;
    config.GlyphMinAdvanceX = 13.0f;
    (void)io.Fonts->AddFontFromMemoryTTF(
        const_cast<void*>(data), static_cast<int>(bytes), 13.0f,
        &config, ranges);
}

void drawPassiveFrame(IDXGISwapChain3* swapChain, UINT, UINT) {
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

    if (g_firstRunWizard) {
        HWND gameWindow{};
        if (swapChain) swapChain->GetHwnd(&gameWindow);
        overlay::set_toggle_suspended(true);
        pollFirstRunKeyboard(gameWindow);
        if (g_firstRunWizard) drawFirstRunWizard();
    }

    // Strong visible indication that this is NOT the interactive editor.
    const ImVec2 origin(18.0f, ImGui::GetIO().DisplaySize.y - 53.0f);
    ImGui::GetForegroundDrawList()->AddRectFilled(
        origin, ImVec2(origin.x + 570.0f, origin.y + 24.0f),
        IM_COL32(17, 17, 17, 220));
    ImGui::GetForegroundDrawList()->AddText(
        ImVec2(origin.x + 8.0f, origin.y + 4.0f),
        IM_COL32(255, 221, 34, 255),
        (g_firstRunWizard
            ? std::string("FIRST-LAUNCH SETUP - ESC HIDES - INPUT UNCHANGED")
            : std::string("RENDER-ONLY PREVIEW - ") + g_toggleLabel +
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
    g_iniPath = folder / L"NCREC.ini";
    g_firstRunWizard = savedKeyFromIni() == 0;
    g_candidateKey = 0;
    g_keysPrimed = false;
    g_wasDown.fill(false);
    g_wasEnterDown = false;
    g_wasEscapeDown = false;
    g_wizardFeedback.clear();
    g_toggleLabel = keyLabel(toggleKey);
    g_ui.status = g_firstRunWizard
        ? "FIRST LAUNCH: select one key, Enter saves. No input capture."
        : "PASSIVE RENDER TEST - " + g_toggleLabel +
          " toggles. Input remains in the game.";
    g_session.setProjectDirectory(folder / L"projects");
    g_lastFrame = std::chrono::steady_clock::now();
    g_styled = false;

    overlay::set_passive_mode(true);
    overlay::set_toggle_key(toggleKey);
    overlay::set_toggle_suspended(g_firstRunWizard);
    overlay::set_visible(g_firstRunWizard);
    overlay::set_font_config_callback(&configureFonts);
    overlay::set_render_callback(&drawPassiveFrame);
    overlay::init();
    g_started = true;
    return true;
}

void stopPassiveFullscreen() {
    if (!g_started) return;
    overlay::set_visible(false);
    overlay::set_toggle_suspended(false);
    g_firstRunWizard = false;
    overlay::deinit();
    g_started = false;
}

} // namespace ncrec::platform::red4ext
