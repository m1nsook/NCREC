#include "platform/GameOverlay.hpp"

#include "core/Session.hpp"

#include <overlay.h>
#include <imgui.h>
#include <dxgi1_4.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace ncrec::platform {
namespace {

constexpr int kInterResourceId = 101;
constexpr int kMonoResourceId = 102;
const ImVec4 kAccent{1.0f, 0.80f, 0.09f, 1.0f};
const ImVec4 kMuted{0.60f, 0.63f, 0.69f, 1.0f};

enum class Page { Timeline, Console, Settings };

ImFont* g_fontBody = nullptr;
ImFont* g_fontHeading = nullptr;
ImFont* g_fontMono = nullptr;

std::filesystem::path moduleDirectory(HMODULE module) {
    std::array<wchar_t, 32768> buffer{};
    const DWORD count = GetModuleFileNameW(
        module, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (!count || count >= buffer.size()) return {};
    return std::filesystem::path(std::wstring(buffer.data(), count)).parent_path();
}

bool isAllowedKey(UINT key) {
    if (key < VK_BACK || key > 0xFE) return false;
    switch (key) {
        case VK_ESCAPE:
        case VK_TAB:
        case VK_RETURN:
        case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
        case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
        case VK_MENU: case VK_LMENU: case VK_RMENU:
        case VK_LWIN: case VK_RWIN:
        case VK_PROCESSKEY: case VK_PACKET:
            return false;
        default:
            return true;
    }
}

bool isLikelyGameKey(UINT key) {
    switch (key) {
        case 'W': case 'A': case 'S': case 'D': case 'E': case 'F':
        case 'R': case 'C': case 'V': case 'I': case 'M':
        case VK_SPACE: case VK_BACK: case VK_CAPITAL:
            return true;
        default:
            return false;
    }
}

std::string keyLabel(UINT key) {
    if (key == VK_OEM_3) return "Grave / tilde (~)";
    if (key == VK_INSERT) return "Insert";
    if (key == VK_SPACE) return "Space";
    if (key >= 'A' && key <= 'Z') return std::string(1, static_cast<char>(key));
    if (key >= '0' && key <= '9') return std::string(1, static_cast<char>(key));
    char result[128]{};
    UINT scan = MapVirtualKeyA(key, MAPVK_VK_TO_VSC);
    if (key == VK_INSERT || key == VK_DELETE || key == VK_HOME ||
        key == VK_END || key == VK_PRIOR || key == VK_NEXT ||
        key == VK_LEFT || key == VK_RIGHT || key == VK_UP || key == VK_DOWN) {
        scan |= 0x100;
    }
    if (scan && GetKeyNameTextA(static_cast<LONG>(scan << 16),
                                 result, static_cast<int>(sizeof(result)))) {
        return result;
    }
    std::snprintf(result, sizeof(result), "Key 0x%02X", key);
    return result;
}

UINT legacyKey(std::wstring_view name) {
    if (name == L"Grave") return VK_OEM_3;
    if (name == L"Insert") return VK_INSERT;
    if (name == L"Home") return VK_HOME;
    if (name == L"End") return VK_END;
    if (name == L"F6") return VK_F6;
    if (name == L"F7") return VK_F7;
    if (name == L"F8") return VK_F8;
    if (name == L"F9") return VK_F9;
    if (name == L"Backspace") return VK_BACK;
    if (name == L"CapsLock") return VK_CAPITAL;
    if (name == L"PageUp") return VK_PRIOR;
    return 0;
}

void applyTheme(float scale) {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(24, 22);
    style.FramePadding = ImVec2(13, 10);
    style.CellPadding = ImVec2(12, 9);
    style.ItemSpacing = ImVec2(12, 12);
    style.ItemInnerSpacing = ImVec2(9, 7);
    style.WindowRounding = 16;
    style.ChildRounding = 12;
    style.FrameRounding = 8;
    style.PopupRounding = 11;
    style.ScrollbarRounding = 8;
    style.GrabRounding = 8;
    style.WindowBorderSize = 1;
    style.ChildBorderSize = 1;
    style.FrameBorderSize = 0;
    style.WindowTitleAlign = ImVec2(0.02f, .5f);

    auto* c = style.Colors;
    c[ImGuiCol_Text] = ImVec4(.94f, .95f, .97f, 1);
    c[ImGuiCol_TextDisabled] = kMuted;
    c[ImGuiCol_WindowBg] = ImVec4(.055f, .059f, .071f, .98f);
    c[ImGuiCol_ChildBg] = ImVec4(.075f, .080f, .093f, 1);
    c[ImGuiCol_PopupBg] = ImVec4(.075f, .080f, .093f, 1);
    c[ImGuiCol_Border] = ImVec4(.18f, .19f, .22f, .75f);
    c[ImGuiCol_FrameBg] = ImVec4(.145f, .15f, .17f, 1);
    c[ImGuiCol_FrameBgHovered] = ImVec4(.20f, .21f, .24f, 1);
    c[ImGuiCol_FrameBgActive] = ImVec4(.23f, .24f, .28f, 1);
    c[ImGuiCol_TitleBg] = ImVec4(.055f, .059f, .071f, 1);
    c[ImGuiCol_TitleBgActive] = ImVec4(.055f, .059f, .071f, 1);
    c[ImGuiCol_Button] = ImVec4(.20f, .21f, .24f, 1);
    c[ImGuiCol_ButtonHovered] = ImVec4(.27f, .28f, .31f, 1);
    c[ImGuiCol_ButtonActive] = ImVec4(.34f, .35f, .38f, 1);
    c[ImGuiCol_Header] = ImVec4(.17f, .18f, .21f, 1);
    c[ImGuiCol_HeaderHovered] = ImVec4(.23f, .24f, .28f, 1);
    c[ImGuiCol_HeaderActive] = ImVec4(.28f, .29f, .31f, 1);
    c[ImGuiCol_CheckMark] = kAccent;
    c[ImGuiCol_SliderGrab] = kAccent;
    c[ImGuiCol_SliderGrabActive] = ImVec4(1, .91f, .45f, 1);
    c[ImGuiCol_Separator] = ImVec4(.20f, .21f, .23f, 1);
    c[ImGuiCol_ResizeGrip] = ImVec4(.38f, .34f, .18f, .4f);
    c[ImGuiCol_ResizeGripHovered] = kAccent;
    c[ImGuiCol_NavHighlight] = kAccent;
    style.ScaleAllSizes(scale);
    ImGui::GetIO().FontGlobalScale = scale;
}

ImFont* addResourceFont(HMODULE module, int id, float pixels) {
    HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!resource) return nullptr;
    HGLOBAL loaded = LoadResource(module, resource);
    if (!loaded) return nullptr;
    void* data = LockResource(loaded);
    const DWORD size = SizeofResource(module, resource);
    if (!data || size == 0) return nullptr;

    ImFontConfig config{};
    config.FontDataOwnedByAtlas = false; // The ASI owns the Win32 resource.
    config.OversampleH = 2;
    config.OversampleV = 2;
    return ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
        data, static_cast<int>(size), pixels, &config);
}

struct EditorState {
    Session session;
    HMODULE module{};
    std::filesystem::path config;
    std::ofstream logfile;
    std::vector<std::string> messages;
    std::array<char, 512> command{};

    std::chrono::steady_clock::time_point previous{std::chrono::steady_clock::now()};
    Page page{Page::Timeline};
    bool firstFrame{true};
    bool onboarding{true};
    bool captureActive{};
    bool shouldStartCapture{};
    bool focusConsole{};
    bool scrollConsole{true};
    float uiScale{1.0f};
    double newKeyframeTime{0.0};
    double nudgeStep{1.0};
    std::string cameraMessage;
    std::array<char, 65> shotName{};
    UINT activeKey{};
    UINT candidateKey{};
    std::string bindingMessage;

    void log(std::string value) {
        if (messages.size() >= 500) messages.erase(messages.begin());
        if (logfile) {
            logfile << value << '\n';
            logfile.flush();
        }
        messages.emplace_back(std::move(value));
    }

    void execute(const char* text) {
        if (!text || !*text) return;
        log(std::string("> ") + text);
        log(session.execute(text));
        scrollConsole = true;
    }

    void initialize(HMODULE instance) {
        module = instance;
        const auto directory = moduleDirectory(instance);
        config = directory / L"NCREC.ini";
        logfile.open(directory / L"NCREC.log", std::ios::app);
        session.setProjectDirectory(directory / L"NCREC" / L"projects");
        std::snprintf(shotName.data(), shotName.size(), "shot_001");

        // Read numeric VK first; support the legacy named toggle key.
        wchar_t setting[80]{};
        GetPrivateProfileStringW(L"Hotkeys", L"ToggleVK", L"",
            setting, static_cast<DWORD>(std::size(setting)), config.c_str());
        if (setting[0]) {
            wchar_t* end = nullptr;
            const unsigned long parsed = std::wcstoul(setting, &end, 10);
            if (end != setting && *end == L'\0' && parsed <= 0xFE &&
                isAllowedKey(static_cast<UINT>(parsed))) {
                activeKey = static_cast<UINT>(parsed);
            }
        }
        if (activeKey == 0) {
            GetPrivateProfileStringW(L"Hotkeys", L"Toggle", L"",
                setting, static_cast<DWORD>(std::size(setting)), config.c_str());
            activeKey = legacyKey(setting);
        }
        onboarding = activeKey == 0;
        // If the first-run setup is hidden with Escape, F10 reopens it.
        // This recovery key is active only until a real binding is saved.
        overlay::set_toggle_key(onboarding ? VK_F10 : activeKey);
        if (onboarding) {
            overlay::set_visible(true);
            shouldStartCapture = true;
            log("NCREC first launch: choose a single key to open the editor.");
        } else {
            log("NCREC: loaded toggle key " + keyLabel(activeKey) + ".");
        }

        const int scalePercent = GetPrivateProfileIntW(
            L"Appearance", L"UIScalePercent", 100, config.c_str());
        uiScale = std::clamp(scalePercent / 100.0f, 0.90f, 1.75f);
        log("NCREC: experimental DX12 overlay. Game camera is not connected.");
    }

    void configureFonts() {
        g_fontBody = addResourceFont(module, kInterResourceId, 17.0f);
        g_fontHeading = addResourceFont(module, kInterResourceId, 23.0f);
        g_fontMono = addResourceFont(module, kMonoResourceId, 15.0f);
        if (!g_fontBody) {
            ImGui::GetIO().Fonts->AddFontDefault();
            log("WARNING: Embedded Inter font unavailable; using ImGui fallback.");
        } else {
            ImGui::GetIO().FontDefault = g_fontBody;
        }
        if (!g_fontHeading) g_fontHeading = ImGui::GetIO().FontDefault;
        if (!g_fontMono) g_fontMono = ImGui::GetIO().FontDefault;
    }

    void beginCapture() {
        candidateKey = 0;
        captureActive = true;
        bindingMessage.clear();
        overlay::begin_key_capture();
    }

    void cancelCapture() {
        if (captureActive) overlay::end_key_capture();
        captureActive = false;
    }

    void pollCapture() {
        if (!captureActive) return;
        const UINT result = overlay::take_captured_key();
        if (result == 0) return;
        captureActive = false;
        if (!isAllowedKey(result)) {
            bindingMessage = "That key is reserved. Try a different single key.";
            shouldStartCapture = false;
            return;
        }
        candidateKey = result;
        bindingMessage.clear();
    }

    void saveBinding(bool exitSetup) {
        if (!isAllowedKey(candidateKey)) return;
        const std::wstring setting = std::to_wstring(candidateKey);
        if (!WritePrivateProfileStringW(
                L"Hotkeys", L"ToggleVK", setting.c_str(), config.c_str())) {
            bindingMessage = "Could not save NCREC.ini. Check folder permissions.";
            return;
        }
        activeKey = candidateKey;
        candidateKey = 0;
        overlay::set_toggle_key(activeKey);
        bindingMessage = "Binding saved.";
        log("NCREC toggle key saved: " + keyLabel(activeKey));
        if (exitSetup) {
            onboarding = false;
            overlay::set_visible(false);
        }
    }

    void saveScale() {
        const int percentage = static_cast<int>(std::lround(uiScale * 100.0f));
        const std::wstring setting = std::to_wstring(percentage);
        if (!WritePrivateProfileStringW(
                L"Appearance", L"UIScalePercent", setting.c_str(), config.c_str())) {
            log("WARNING: Could not save UI scale.");
        }
    }

    void sectionHeading(const char* heading, const char* caption) const {
        if (g_fontHeading) ImGui::PushFont(g_fontHeading);
        ImGui::TextUnformatted(heading);
        if (g_fontHeading) ImGui::PopFont();
        if (caption) ImGui::TextColored(kMuted, "%s", caption);
    }

    bool accentButton(const char* label, const ImVec2& size = ImVec2(0, 0)) const {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.94f, .73f, .06f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, .82f, .25f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.79f, .62f, .05f, 1));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(.07f, .07f, .07f, 1));
        const bool clicked = ImGui::Button(label, size);
        ImGui::PopStyleColor(4);
        return clicked;
    }

    void bindingPicker(bool setup) {
        if (captureActive) {
            ImGui::TextColored(kAccent, "Listening... press one key");
        } else if (candidateKey != 0) {
            ImGui::Text("Selected: %s", keyLabel(candidateKey).c_str());
        } else if (setup) {
            ImGui::TextColored(kMuted, "Choose a comfortable key. No Ctrl, Shift or Fn.");
        } else {
            ImGui::Text("Current: %s", keyLabel(activeKey).c_str());
        }

        if (candidateKey != 0 && isLikelyGameKey(candidateKey)) {
            ImGui::TextColored(kAccent,
                "This key may conflict with Cyberpunk's normal controls.");
        }
        if (!bindingMessage.empty()) {
            ImGui::TextColored(kMuted, "%s", bindingMessage.c_str());
        }

        if (captureActive) {
            if (ImGui::Button("Cancel capture")) cancelCapture();
        } else if (ImGui::Button(candidateKey ? "Choose again" : "Press a key")) {
            beginCapture();
        }

        if (candidateKey) {
            ImGui::SameLine();
            // Keyboard fallback: choose a key, then press Enter to save.
            if (accentButton(setup ? "Save and finish" : "Save binding")
                || (setup && !captureActive && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
                saveBinding(setup);
            }
            if (setup) {
                ImGui::TextColored(kMuted, "Press Enter to save without using the mouse.");
            }
        }
    }

    void renderWelcome() {
        if (shouldStartCapture) {
            beginCapture();
            shouldStartCapture = false;
        }

        const ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(
            ImVec2(io.DisplaySize.x * .5f, io.DisplaySize.y * .5f),
            ImGuiCond_Always, ImVec2(.5f, .5f));
        ImGui::SetNextWindowSize(
            ImVec2(540 * uiScale, 345 * uiScale), ImGuiCond_Always);
        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("NCREC First Run##ncrecSetup", nullptr, flags);

        ImGui::TextColored(kAccent, "NCREC  /  FIRST LAUNCH");
        ImGui::Dummy(ImVec2(0, 10 * uiScale));
        sectionHeading("Welcome to NCREC",
            "Your cinematic camera workspace for Night City.");
        ImGui::Dummy(ImVec2(0, 10 * uiScale));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 8 * uiScale));
        ImGui::TextUnformatted("Choose a key to open and close the editor");
        ImGui::TextColored(kMuted,
            "Press one key, then press Enter to save without the mouse.");
        ImGui::Dummy(ImVec2(0, 12 * uiScale));
        bindingPicker(true);
        ImGui::Dummy(ImVec2(0, 16 * uiScale));
        ImGui::Separator();
        ImGui::TextColored(kMuted, "Esc hides setup; F10 reopens it if needed.");
        ImGui::TextColored(kMuted, "No account, external font install or CET window required.");
        ImGui::End();
    }

    void drawTimeline() {
        sectionHeading("Camera timeline", "Build and preview camera shots before live engine integration.");
        ImGui::TextColored(kAccent,
            "VIRTUAL CAMERA  |  No Cyberpunk camera read/write is enabled yet.");
        ImGui::Dummy(ImVec2(0, 5 * uiScale));
        if (ImGui::BeginChild("TimelineControls", ImVec2(0, 136 * uiScale), ImGuiChildFlags_Borders)) {
            if (accentButton("Load demo")) {
                execute("path demo");
                newKeyframeTime = 5.0;
            }
            ImGui::SameLine();
            if (ImGui::Button(session.playing() ? "Pause preview" : "Play preview")) {
                execute(session.playing() ? "path pause" : "path play");
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear shot")) execute("path clear");

            float time = static_cast<float>(session.cursorSeconds());
            const float duration = static_cast<float>(session.path().durationSeconds());
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderFloat("##TimelineTime", &time, 0.0f,
                                   std::max(duration, .01f), "Timeline  %.3f s")) {
                session.seek(time);
            }
            ImGui::TextColored(kMuted, "%zu points  /  %.3f seconds",
                session.path().size(), duration);
        }
        ImGui::EndChild();

        ImGui::Dummy(ImVec2(0, 7 * uiScale));
        if (ImGui::BeginChild("CameraPreview", ImVec2(0, 212 * uiScale), ImGuiChildFlags_Borders)) {
            ImGui::TextColored(kAccent, "VIRTUAL CAMERA POSE");
            ImGui::TextColored(kMuted, "Edit coordinates to prepare a shot. Not affecting the game.");
            CameraState pose = session.rig().pose();
            bool changed = false;
            ImGui::SetNextItemWidth(-1);
            changed |= ImGui::InputDouble("X##pose", &pose.position.x, 0.1, 1.0, "%.3f");
            ImGui::SetNextItemWidth(-1);
            changed |= ImGui::InputDouble("Y##pose", &pose.position.y, 0.1, 1.0, "%.3f");
            ImGui::SetNextItemWidth(-1);
            changed |= ImGui::InputDouble("Z##pose", &pose.position.z, 0.1, 1.0, "%.3f");
            float fov = static_cast<float>(pose.fovDegrees);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderFloat("FOV##pose", &fov, 5.0f, 120.0f, "%.1f deg")) {
                pose.fovDegrees = fov;
                changed = true;
            }
            if (changed && !session.setCameraPose(pose)) {
                cameraMessage = "Invalid pose: use finite coordinates and FOV from 0-180.";
            }
        }
        ImGui::EndChild();

        ImGui::Dummy(ImVec2(0, 7 * uiScale));
        if (ImGui::BeginChild("KeyframeEditor", ImVec2(0, 235 * uiScale), ImGuiChildFlags_Borders)) {
            ImGui::TextColored(kAccent, "KEYFRAMES");
            ImGui::SetNextItemWidth(175 * uiScale);
            ImGui::InputDouble("Time (seconds)", &newKeyframeTime, 0.25, 1.0, "%.3f");
            if (accentButton("Add virtual pose")) {
                if (session.addKeyframe(newKeyframeTime)) {
                    cameraMessage = "Keyframe captured. Set another time to add the next.";
                } else {
                    cameraMessage = "Could not add: time must be nonnegative and unique.";
                }
            }
            ImGui::Separator();
            if (session.path().size() == 0) {
                ImGui::TextColored(kMuted, "No points yet. Set your pose and click Add.");
            }
            // Copy the entry before removing it; erase invalidates the path vector.
            for (std::size_t i = 0; i < session.path().size(); ++i) {
                const Keyframe frame = session.path().keyframes()[i];
                ImGui::PushID(static_cast<int>(i));
                ImGui::Text("%zu   %.3f s", i, frame.timeSeconds);
                ImGui::SameLine(220 * uiScale);
                if (ImGui::SmallButton("Edit pose")) {
                    (void)session.setCameraPose(frame.camera);
                    newKeyframeTime = frame.timeSeconds;
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Delete")) {
                    (void)session.removeKeyframe(i);
                    cameraMessage = "Keyframe removed.";
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
            }
        }
        ImGui::EndChild();

        ImGui::Dummy(ImVec2(0, 7 * uiScale));
        if (ImGui::BeginChild("Projects", ImVec2(0, 119 * uiScale), ImGuiChildFlags_Borders)) {
            ImGui::TextColored(kAccent, "SAVE / LOAD SHOT");
            ImGui::SetNextItemWidth(240 * uiScale);
            ImGui::InputTextWithHint("##ShotFile", "shot_001", shotName.data(), shotName.size());
            ImGui::SameLine();
            if (ImGui::Button("Save")) {
                const std::string command = "path save " + std::string(shotName.data());
                cameraMessage = session.execute(command);
                log(cameraMessage);
            }
            ImGui::SameLine();
            if (ImGui::Button("Load")) {
                const std::string command = "path load " + std::string(shotName.data());
                cameraMessage = session.execute(command);
                log(cameraMessage);
            }
            ImGui::TextColored(kMuted, "Projects: bin/x64/plugins/NCREC/projects/");
        }
        ImGui::EndChild();

        if (!cameraMessage.empty()) ImGui::TextColored(kMuted, "%s", cameraMessage.c_str());
        if (g_fontMono) ImGui::PushFont(g_fontMono);
        const CameraState& pose = session.rig().pose();
        ImGui::Text("Preview XYZ %.2f / %.2f / %.2f   FOV %.1f",
            pose.position.x, pose.position.y, pose.position.z, pose.fovDegrees);
        if (g_fontMono) ImGui::PopFont();
    }

    void drawConsole() {
        sectionHeading("Console", "Local commands and diagnostic messages.");
        ImGui::Dummy(ImVec2(0, 5 * uiScale));
        ImGui::BeginChild("ConsoleText", ImVec2(0, -56 * uiScale), ImGuiChildFlags_Borders);
        if (g_fontMono) ImGui::PushFont(g_fontMono);
        for (const auto& value : messages) {
            ImGui::TextUnformatted(value.c_str());
        }
        if (scrollConsole) {
            ImGui::SetScrollHereY(1.0f);
            scrollConsole = false;
        }
        if (g_fontMono) ImGui::PopFont();
        ImGui::EndChild();
        if (focusConsole) {
            ImGui::SetKeyboardFocusHere();
            focusConsole = false;
        }
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputTextWithHint("##command", "Enter NCREC command...",
                command.data(), command.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
            execute(command.data());
            command.fill('\0');
            ImGui::SetKeyboardFocusHere(-1);
        }
    }

    void drawSettings() {
        sectionHeading("Settings", "Personalize your cinematic workspace.");
        ImGui::Dummy(ImVec2(0, 8 * uiScale));

        if (ImGui::BeginChild("ControlsPanel", ImVec2(0, 185 * uiScale), ImGuiChildFlags_Borders)) {
            ImGui::TextColored(kAccent, "CONTROLS");
            ImGui::Text("Toggle editor: %s", keyLabel(activeKey).c_str());
            bindingPicker(false);
        }
        ImGui::EndChild();

        ImGui::Dummy(ImVec2(0, 12 * uiScale));
        if (ImGui::BeginChild("AppearancePanel", ImVec2(0, 175 * uiScale), ImGuiChildFlags_Borders)) {
            ImGui::TextColored(kAccent, "APPEARANCE");
            ImGui::TextUnformatted("Dark graphite + NCREC gold");
            ImGui::TextColored(kMuted, "Inter interface / JetBrains Mono console");
            float scale = uiScale;
            ImGui::SetNextItemWidth(250 * uiScale);
            if (ImGui::SliderFloat("UI scale", &scale, .90f, 1.75f, "%.2fx")) {
                uiScale = scale;
                applyTheme(uiScale);
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) saveScale();
        }
        ImGui::EndChild();

        ImGui::Dummy(ImVec2(0, 12 * uiScale));
        ImGui::TextColored(kMuted,
            "OBS controls and lens settings will appear here once implemented.");
    }

    void render() {
        if (firstFrame) {
            ImGui::GetIO().IniFilename = nullptr;
            applyTheme(uiScale);
            firstFrame = false;
        }
        pollCapture();

        const auto now = std::chrono::steady_clock::now();
        session.advance(std::chrono::duration<double>(now - previous).count());
        previous = now;

        if (onboarding) {
            renderWelcome();
            return; // Do not let Escape hide the mandatory first-run setup.
        }

        ImGui::SetNextWindowSize(ImVec2(940 * uiScale, 680 * uiScale),
                                 ImGuiCond_FirstUseEver);
        bool open = true;
        if (ImGui::Begin("NCREC  |  Cinematic Editor", &open)) {
            ImGui::TextColored(kAccent, "NCREC");
            ImGui::SameLine();
            ImGui::TextColored(kMuted, " /  NIGHT CITY CINEMATIC TOOLS");
            ImGui::SameLine(ImGui::GetContentRegionAvail().x > 200.0f
                                ? ImGui::GetWindowWidth() - 120.0f * uiScale : 0.0f);
            if (ImGui::Button("Hide UI")) overlay::set_visible(false);
            ImGui::Separator();

            auto tab = [&](const char* label, Page pageValue) {
                const bool active = page == pageValue;
                if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.36f, .29f, .08f, 1));
                if (ImGui::Button(label, ImVec2(135 * uiScale, 0))) {
                    cancelCapture();
                    page = pageValue;
                }
                if (active) ImGui::PopStyleColor();
            };
            tab("Timeline", Page::Timeline);
            ImGui::SameLine();
            tab("Console", Page::Console);
            ImGui::SameLine();
            tab("Settings", Page::Settings);
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 5 * uiScale));

            if (page == Page::Timeline) drawTimeline();
            else if (page == Page::Console) drawConsole();
            else drawSettings();
        }
        ImGui::End();

        if (!open || (!captureActive && ImGui::IsKeyPressed(ImGuiKey_Escape))) {
            cancelCapture();
            overlay::set_visible(false);
        }
    }
};

EditorState g_editor;

void configureFonts() {
    g_editor.configureFonts();
}

void drawFrame(IDXGISwapChain3*, UINT, UINT) {
    g_editor.render();
}

} // namespace

DWORD runOverlay(HMODULE module) {
    g_editor.initialize(module);
    overlay::set_font_config_callback(&configureFonts);
    overlay::set_render_callback(&drawFrame);
    overlay::init();
    return 0; // ASI stays resident; hot-unloading is not supported.
}

} // namespace ncrec::platform
