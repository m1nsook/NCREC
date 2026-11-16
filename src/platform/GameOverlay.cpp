#include "platform/GameOverlay.hpp"

#include "core/Session.hpp"

#include <overlay.h>
#include <imgui.h>
#include <dxgi1_4.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace ncrec::platform {
namespace {

struct HotkeyChoice {
    const char* label;
    const wchar_t* setting;
    UINT virtualKey;
};

constexpr std::array<HotkeyChoice, 11> kHotkeys{{
    {"Grave / tilde (left of 1)", L"Grave", VK_OEM_3},
    {"Insert", L"Insert", VK_INSERT},
    {"Home", L"Home", VK_HOME},
    {"End", L"End", VK_END},
    {"F6", L"F6", VK_F6},
    {"F7", L"F7", VK_F7},
    {"F8", L"F8", VK_F8},
    {"F9", L"F9", VK_F9},
    {"Backspace", L"Backspace", VK_BACK},
    {"Caps Lock", L"CapsLock", VK_CAPITAL},
    {"Page Up", L"PageUp", VK_PRIOR},
}};

std::filesystem::path moduleDirectory(HMODULE module) {
    std::array<wchar_t, 32768> buffer{};
    const DWORD count = GetModuleFileNameW(
        module, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (!count || count >= buffer.size()) return {};
    return std::filesystem::path(std::wstring(buffer.data(), count)).parent_path();
}

struct EditorState {
    Session session;
    std::filesystem::path config;
    std::ofstream logfile;
    std::vector<std::string> messages;
    std::array<char, 512> command{};
    std::chrono::steady_clock::time_point previous{std::chrono::steady_clock::now()};
    bool firstFrame{true};
    bool focusConsole{};
    bool scrollConsole{true};
    int selectedHotkey{};

    void log(std::string message) {
        if (messages.size() >= 500) {
            messages.erase(messages.begin());
        }
        if (logfile) {
            logfile << message << '\n';
            logfile.flush();
        }
        messages.push_back(std::move(message));
    }

    void execute(const char* value) {
        if (!value || !*value) return;
        log(std::string("> ") + value);
        log(session.execute(value));
        scrollConsole = true;
    }

    void initialize(HMODULE module) {
        const auto base = moduleDirectory(module);
        config = base / L"NCREC.ini";
        logfile.open(base / L"NCREC.log", std::ios::app);

        wchar_t setting[64]{};
        GetPrivateProfileStringW(
            L"Hotkeys", L"Toggle", L"Grave",
            setting, static_cast<DWORD>(std::size(setting)), config.c_str());
        for (int i = 0; i < static_cast<int>(kHotkeys.size()); ++i) {
            if (std::wstring_view(setting) == kHotkeys[i].setting) {
                selectedHotkey = i;
                break;
            }
        }
        overlay::set_toggle_key(kHotkeys[selectedHotkey].virtualKey);
        log("NCREC: experimental DX12 in-game ImGui renderer (no extra window).");
        log("Toggle with one key; open console directly in the editor.");
        log("Game camera adapter is NOT implemented. 'path demo' previews math.");
    }

    void setHotkey(int index) {
        if (index < 0 || index >= static_cast<int>(kHotkeys.size())) return;
        overlay::set_toggle_key(kHotkeys[index].virtualKey);
        selectedHotkey = index;
        if (!WritePrivateProfileStringW(
                L"Hotkeys", L"Toggle", kHotkeys[index].setting, config.c_str())) {
            log("WARNING: Could not save NCREC.ini.");
        }
    }

    void render() {
        if (firstFrame) {
            ImGui::StyleColorsDark();
            ImGui::GetIO().IniFilename = nullptr;
            auto& style = ImGui::GetStyle();
            style.WindowRounding = 6.0f;
            style.FrameRounding = 4.0f;
            style.Colors[ImGuiCol_Button] = ImVec4(.43f, .33f, .03f, 1.0f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(.70f, .51f, .05f, 1.0f);
            firstFrame = false;
        }

        const auto now = std::chrono::steady_clock::now();
        session.advance(std::chrono::duration<double>(now - previous).count());
        previous = now;

        ImGui::SetNextWindowPos(ImVec2(80, 60), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(900, 640), ImGuiCond_FirstUseEver);

        bool open = true;
        if (ImGui::Begin("NCREC | Cinematic Camera Editor (DX12 experimental)", &open)) {
            ImGui::TextColored(ImVec4(1.0f, .83f, .05f, 1.0f), "NCREC");
            ImGui::SameLine();
            ImGui::TextDisabled("Native in-game editor | CET-independent");
            ImGui::SameLine();
            if (ImGui::Button("Hide UI")) overlay::set_visible(false);
            ImGui::SameLine();
            if (ImGui::Button("Focus console")) focusConsole = true;

            int selected = selectedHotkey;
            if (ImGui::Combo("Toggle key (one press)", &selected,
                [](void* /*data*/, int index) -> const char* {
                    return kHotkeys[index].label;
                }, nullptr, static_cast<int>(kHotkeys.size()))) {
                setHotkey(selected);
            }
            ImGui::TextDisabled("Esc or your selected toggle key hides the editor.");
            ImGui::Separator();

            ImGui::TextWrapped(
                "PROTOTYPE: the DX12 renderer is experimental and camera "
                "controls are NOT connected to the game yet. Playback below "
                "previews camera-path interpolation only.");

            if (ImGui::Button("Load demo path")) execute("path demo");
            ImGui::SameLine();
            if (ImGui::Button(session.playing() ? "Pause" : "Play")) {
                execute(session.playing() ? "path pause" : "path play");
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear")) execute("path clear");

            float time = static_cast<float>(session.cursorSeconds());
            const float duration = static_cast<float>(session.path().durationSeconds());
            if (ImGui::SliderFloat("Timeline (s)", &time, 0.0f,
                                   std::max(duration, 0.01f), "%.3f")) {
                session.seek(time);
            }
            ImGui::Text("Keyframes: %zu | Duration: %.3fs | %s",
                session.path().size(), duration,
                session.playing() ? "Playing" : "Paused");
            if (const auto sample = session.path().evaluate(session.cursorSeconds())) {
                ImGui::Text("Position (%.3f, %.3f, %.3f), FOV %.1f",
                    sample->position.x, sample->position.y,
                    sample->position.z, sample->fovDegrees);
            }

            ImGui::SeparatorText("Console");
            const float commandHeight = ImGui::GetFrameHeightWithSpacing() * 2.1f;
            ImGui::BeginChild("console output", ImVec2(0, -commandHeight), true);
            for (const auto& line : messages) {
                ImGui::TextUnformatted(line.c_str());
            }
            if (scrollConsole) {
                ImGui::SetScrollHereY(1.0f);
                scrollConsole = false;
            }
            ImGui::EndChild();

            if (focusConsole) {
                ImGui::SetKeyboardFocusHere();
                focusConsole = false;
            }
            if (ImGui::InputText("##command", command.data(), command.size(),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
                execute(command.data());
                command.fill('\0');
                ImGui::SetKeyboardFocusHere(-1);
            }
            ImGui::SameLine();
            ImGui::TextDisabled("help / path demo / path play");
        }
        ImGui::End();

        if (!open || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            overlay::set_visible(false);
        }
    }
};

EditorState g_editor;

void drawFrame(IDXGISwapChain3*, UINT, UINT) {
    g_editor.render();
}

} // namespace

DWORD runOverlay(HMODULE module) {
    g_editor.initialize(module);
    overlay::set_render_callback(&drawFrame);
    overlay::init(); // DXGI factory intercept; UI draws into game backbuffers.
    return 0; // ASI stays loaded for the process lifetime; no hot unloading.
}

} // namespace ncrec::platform
