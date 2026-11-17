#include "editor/FullscreenEditor.hpp"

#include "editor/WorkspaceLayout.hpp"
#include "editor/FontAwesomeIcons.hpp"
#include "core/Session.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace ncrec::editor {
namespace {

// Direct translation of design/demo.html's CSS palette.
constexpr ImU32 kMain32   = IM_COL32(15, 15, 15, 255);
constexpr ImU32 kPanel32  = IM_COL32(26, 26, 26, 255);
constexpr ImU32 kHeader32 = IM_COL32(20, 20, 20, 255);
constexpr ImU32 kInput32  = IM_COL32(42, 42, 42, 255);
constexpr ImU32 kBorder32 = IM_COL32(51, 51, 51, 255);
constexpr ImU32 kMuted32  = IM_COL32(136, 136, 136, 255);
constexpr ImU32 kText32   = IM_COL32(224, 224, 224, 255);
constexpr ImU32 kAccent32 = IM_COL32(252, 238, 10, 255);
constexpr ImU32 kRed32    = IM_COL32(255, 85, 85, 255);
constexpr ImU32 kGreen32  = IM_COL32(85, 255, 85, 255);
constexpr ImU32 kBlue32   = IM_COL32(85, 85, 255, 255);
const ImVec4 kMain{15.0f / 255, 15.0f / 255, 15.0f / 255, 1};
const ImVec4 kPanel{26.0f / 255, 26.0f / 255, 26.0f / 255, 1};
const ImVec4 kHeader{20.0f / 255, 20.0f / 255, 20.0f / 255, 1};
const ImVec4 kInput{42.0f / 255, 42.0f / 255, 42.0f / 255, 1};
const ImVec4 kMuted{136.0f / 255, 136.0f / 255, 136.0f / 255, 1};
const ImVec4 kAccent{252.0f / 255, 238.0f / 255, 10.0f / 255, 1};

constexpr ImGuiWindowFlags kFixedFlags =
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
    ImGuiWindowFlags_NoSavedSettings;

ImVec2 plus(ImVec2 a, float dx, float dy) { return {a.x + dx, a.y + dy}; }

// Missing icon resource must leave a readable text-only editor.
bool iconsLoaded() {
    ImFont* font = ImGui::GetFont();
    return font && font->FindGlyphNoFallback(0xf03d) != nullptr;
}
std::string withIcon(const char* glyph, const char* label) {
    return iconsLoaded() ? std::string(glyph) + "  " + label : label;
}
void drawIconAt(ImDrawList* d, ImVec2 center, const char* glyph,
                ImU32 color, float size) {
    ImFont* font = ImGui::GetFont();
    const ImVec2 extent = font->CalcTextSizeA(size, 10000.0f, 0.0f, glyph);
    d->AddText(font, size, plus(center, -extent.x * .5f, -extent.y * .5f),
               color, glyph);
}

void smallText(ImDrawList* d, ImVec2 p, ImU32 color, const char* value, float size = 11.0f) {
    d->AddText(ImGui::GetFont(), size, p, color, value);
}

void centeredText(ImDrawList* d, ImVec2 center, ImU32 color, const char* label,
                  float size = 13.0f) {
    ImFont* font = ImGui::GetFont();
    const ImVec2 extent = font->CalcTextSizeA(size, 10000, 0, label);
    d->AddText(font, size, plus(center, -extent.x * 0.5f, -extent.y * 0.5f),
               color, label);
}

void addMessage(FullscreenEditorState& ui, std::string message) {
    ui.status = message;
    if (ui.consoleLines.size() >= 250) ui.consoleLines.erase(ui.consoleLines.begin());
    ui.consoleLines.push_back(std::move(message));
}

void runCommand(Session& session, FullscreenEditorState& ui, const std::string& command) {
    addMessage(ui, "> " + command);
    addMessage(ui, session.execute(command));
}

void saveShot(Session& session, FullscreenEditorState& ui) {
    runCommand(session, ui, std::string("path save ") + ui.shotName);
}
void loadShot(Session& session, FullscreenEditorState& ui) {
    runCommand(session, ui, std::string("path load ") + ui.shotName);
}

bool primaryButton(const char* label, ImVec2 size = ImVec2(0, 0)) {
    ImGui::PushStyleColor(ImGuiCol_Button, kAccent);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.90f, .85f, .04f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.82f, .76f, .02f, 1));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
    const bool pressed = ImGui::Button(label, size);
    ImGui::PopStyleColor(4);
    return pressed;
}

bool beginPanel(const char* id, Rect rect, ImVec4 background) {
    if (rect.width <= 0 || rect.height <= 0) return false;
    ImGui::SetCursorScreenPos(ImVec2(rect.x, rect.y));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, background);
    ImGui::BeginChild(id, ImVec2(rect.width, rect.height),
                      ImGuiChildFlags_Borders, kFixedFlags);
    return true;
}

void endPanel() {
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

// The header is an actual 32px surface, not a Text+Separator widget.
void panelHeading(Rect rect, const char* title, const char* glyph = nullptr) {
    const ImVec2 p(rect.x, rect.y);
    ImDrawList* d = ImGui::GetWindowDrawList();
    d->AddRectFilled(p, plus(p, rect.width, 32), kHeader32);
    d->AddLine(plus(p, 0, 31.5f), plus(p, rect.width, 31.5f), kBorder32);
    if (glyph && iconsLoaded()) {
        smallText(d, plus(p, 12, 9), kMuted32, glyph, 12);
        smallText(d, plus(p, 31, 9), kMuted32, title);
    } else {
        smallText(d, plus(p, 12, 9), kMuted32, title);
    }
    ImGui::SetCursorScreenPos(plus(p, 16, 48));
}

void drawCameraGlyph(ImDrawList* d, ImVec2 c, float scale, ImU32 color) {
    // Camera icon: protruding viewfinder, rounded body and lens.
    const ImVec2 p = plus(c, -25 * scale, -15 * scale);
    d->AddRectFilled(plus(p, 8 * scale, -8 * scale),
                     plus(p, 23 * scale, 0), color, 3 * scale);
    d->AddRectFilled(p, plus(p, 50 * scale, 34 * scale), color, 5 * scale);
    d->AddCircleFilled(plus(p, 25 * scale, 17 * scale), 10 * scale, kMain32);
    d->AddCircleFilled(plus(p, 25 * scale, 17 * scale), 6 * scale, color);
}

void drawFilmGlyph(ImDrawList* d, ImVec2 c) {
    const ImVec2 p = plus(c, -17, -13);
    d->AddRectFilled(p, plus(p, 34, 26), IM_COL32(92, 92, 92, 255), 4);
    for (int i = 0; i < 3; ++i) {
        d->AddRectFilled(plus(p, 4 + i * 10.5f, 4), plus(p, 9 + i * 10.5f, 10), kPanel32, 1);
        d->AddRectFilled(plus(p, 4 + i * 10.5f, 17), plus(p, 9 + i * 10.5f, 23), kPanel32, 1);
    }
    d->AddRectFilled(plus(p, 4, 12), plus(p, 30, 15), kPanel32);
}

void dashedRect(ImDrawList* d, ImVec2 a, ImVec2 b, ImU32 col) {
    constexpr float dash = 5, gap = 5;
    for (float x = a.x; x < b.x; x += dash + gap) {
        d->AddLine({x, a.y}, {std::min(x + dash, b.x), a.y}, col);
        d->AddLine({x, b.y}, {std::min(x + dash, b.x), b.y}, col);
    }
    for (float y = a.y; y < b.y; y += dash + gap) {
        d->AddLine({a.x, y}, {a.x, std::min(y + dash, b.y)}, col);
        d->AddLine({b.x, y}, {b.x, std::min(y + dash, b.y)}, col);
    }
}

void drawTopBar(Session& session, FullscreenEditorState& ui, Rect rect) {
    if (!beginPanel("##NCRECTopBar", rect, kHeader)) return;
    ImGui::SetCursorScreenPos({rect.x + 16, rect.y + 9});
    ImGui::TextColored(kAccent, "%s", withIcon(fa::video, "NCREC").c_str());
    ImGui::SameLine(112);
    ImGui::SetCursorPosY(5);
    if (ImGui::Button(withIcon(fa::file, "File").c_str())) ImGui::OpenPopup("NCRECFile");
    if (ImGui::BeginPopup("NCRECFile")) {
        if (ImGui::MenuItem("New shot")) runCommand(session, ui, "path clear");
        if (ImGui::MenuItem("Save shot")) saveShot(session, ui);
        if (ImGui::MenuItem("Load shot")) loadShot(session, ui);
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(withIcon(fa::edit, "Edit").c_str())) ImGui::OpenPopup("NCRECEdit");
    if (ImGui::BeginPopup("NCRECEdit")) {
        ImGui::MenuItem("Undo (coming later)", nullptr, false, false);
        ImGui::MenuItem("Redo (coming later)", nullptr, false, false);
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(withIcon(fa::eye, "View").c_str())) ImGui::OpenPopup("NCRECView");
    if (ImGui::BeginPopup("NCRECView")) {
        if (ImGui::MenuItem("Inspector: Timeline")) ui.inspector = InspectorTab::Camera;
        if (ImGui::MenuItem("Inspector: Console")) ui.inspector = InspectorTab::Console;
        if (ImGui::MenuItem("Inspector: Settings")) ui.inspector = InspectorTab::Settings;
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(withIcon(fa::play, "Playback").c_str()))
        runCommand(session, ui, session.playing() ? "path pause" : "path play");
    ImGui::SameLine();
    if (ImGui::Button(withIcon(fa::window, "Window").c_str())) ImGui::OpenPopup("NCRECWindow");
    if (ImGui::BeginPopup("NCRECWindow")) {
        ImGui::TextDisabled("Fullscreen workspace");
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(withIcon(fa::help, "Help").c_str())) ImGui::OpenPopup("NCRECHelp");
    if (ImGui::BeginPopup("NCRECHelp")) {
        ImGui::TextDisabled("NCREC — RED4ext editor preview");
        ImGui::TextDisabled("Game camera is not connected yet.");
        ImGui::EndPopup();
    }

    // Correct right-aligned save + hide controls, as in the HTML.
    if (rect.width > 820) {
        ImGui::SetCursorScreenPos({rect.x + rect.width - 206, rect.y + 5});
        if (ImGui::Button(withIcon(fa::save, "Save").c_str())) saveShot(session, ui);
        ImGui::SameLine();
        if (ImGui::Button(withIcon(fa::desktop, "Hide UI").c_str())) ui.closeRequested = true;
    }
    endPanel();
}

void drawProjects(Session& session, FullscreenEditorState& ui, Rect rect) {
    if (!beginPanel("##NCRECProjects", rect, kPanel)) return;
    panelHeading(rect, "PROJECT / SHOTS", fa::folderOpen);

    const bool empty = session.path().size() == 0;
    if (empty && rect.height > 320) {
        ImDrawList* d = ImGui::GetWindowDrawList();
        const float center = rect.x + rect.width * .5f;
        const float upper = rect.y + std::min(160.0f, rect.height * .28f);
        if (iconsLoaded())
            drawIconAt(d, {center, upper}, fa::film,
                       IM_COL32(92, 92, 92, 255), 28.0f);
        else
            drawFilmGlyph(d, {center, upper});
        centeredText(d, {center, upper + 48}, kMuted32, "No shots created yet.", 12);
        ImGui::SetCursorScreenPos({rect.x + 16, upper + 72});
    }
    else {
        ImGui::TextColored(kMuted, "CURRENT SHOT");
        ImGui::Text("%zu camera keyframe(s)", session.path().size());
        ImGui::Dummy({0, 5});
    }

    ImGui::SetCursorPosX(16);
    if (primaryButton(withIcon(fa::add, "New Shot").c_str(), {std::max(40.0f, rect.width - 32), 30}))
        runCommand(session, ui, "path clear");

    ImGui::Dummy({0, 8});
    ImGui::SetCursorPosX(16);
    ImGui::TextColored(kMuted, "Shot filename");
    ImGui::SetCursorPosX(16);
    ImGui::SetNextItemWidth(std::max(40.0f, rect.width - 32));
    ImGui::InputTextWithHint("##shotName", "shot_001", ui.shotName, sizeof(ui.shotName));

    ImGui::SetCursorPosX(16);
    if (ImGui::Button(withIcon(fa::save, "Save Shot").c_str(), {std::max(40.0f, rect.width - 32), 29}))
        saveShot(session, ui);
    ImGui::SetCursorPosX(16);
    if (ImGui::Button(withIcon(fa::folderOpen, "Load Shot").c_str(), {std::max(40.0f, rect.width - 32), 29}))
        loadShot(session, ui);
    endPanel();
}

void drawPreview(Rect rect) {
    if (!beginPanel("##NCRECPreview", rect, ImVec4(.020f, .020f, .020f, 1))) return;
    ImDrawList* d = ImGui::GetWindowDrawList();

    const ImVec2 top = {rect.x + 12, rect.y + 12};
    const ImVec2 bottom = {rect.x + rect.width - 12, rect.y + rect.height - 12};
    if (bottom.x > top.x && bottom.y > top.y) {
        dashedRect(d, top, bottom, kBorder32);
    }

    // Floating viewport-tool strip: select, pan, orbit, zoom and grid.
    const ImVec2 bar{rect.x + 24, rect.y + 24};
    d->AddRectFilled(bar, plus(bar, 190, 38), IM_COL32(26, 26, 26, 232), 4);
    d->AddRect(bar, plus(bar, 190, 38), kBorder32, 4);
    const char* tools[] = {fa::select, fa::pan, fa::orbit, fa::zoom, fa::grid};
    const char* fallback[] = {"S", "H", "O", "+", "#"};
    for (int i = 0; i < 5; ++i) {
        ImVec2 cell = plus(bar, 4 + i * 37, 4);
        if (i == 0) d->AddRectFilled(cell, plus(cell, 30, 30),
                                    IM_COL32(61, 56, 18, 255), 4);
        centeredText(d, plus(cell, 15, 15), i == 0 ? kAccent32 : kMuted32,
                     iconsLoaded() ? tools[i] : fallback[i], 14);
    }

    const ImVec2 center{rect.x + rect.width * .5f, rect.y + rect.height * .5f};
    if (iconsLoaded())
        drawIconAt(d, plus(center, 0, -24), fa::camera,
                   IM_COL32(74, 74, 74, 255), 34.0f);
    else
        drawCameraGlyph(d, plus(center, 0, -24), 1.0f,
                        IM_COL32(74, 74, 74, 255));
    centeredText(d, plus(center, 0, 27), kMuted32, "[ Camera Preview Feed ]", 13);
    centeredText(d, plus(center, 0, 47), IM_COL32(94, 94, 94, 255),
                 "Game camera not yet connected", 11);
    endPanel();
}

void drawPlayback(Session& session, FullscreenEditorState& ui, Rect rect) {
    if (!beginPanel("##NCRECPlayback", rect, kPanel)) return;
    // Toolbar is centered horizontally beneath the preview.
    constexpr float groupWidth = 536;
    const float x = rect.x + std::max(10.0f, (rect.width - groupWidth) * .5f);
    ImGui::SetCursorScreenPos({x, rect.y + 7});
    if (ImGui::Button(withIcon(fa::folderOpen, "Load Demo").c_str(), {112, 28})) runCommand(session, ui, "path demo");
    ImGui::SameLine();
    if (ImGui::Button(iconsLoaded() ? fa::back : "|<", {36, 28})) session.seek(0.0);
    ImGui::SameLine();
    if (primaryButton(withIcon(session.playing() ? fa::pause : fa::play,
                                   session.playing() ? "Pause" : "Play Preview").c_str(),
                      {145, 28}))
        runCommand(session, ui, session.playing() ? "path pause" : "path play");
    ImGui::SameLine();
    if (ImGui::Button(iconsLoaded() ? fa::next : ">|", {36, 28})) session.seek(session.path().durationSeconds());
    ImGui::SameLine();
    if (ImGui::Button(withIcon(fa::trash, "Clear Shot").c_str(), {111, 28})) runCommand(session, ui, "path clear");
    endPanel();
}

bool axisInput(const char* label, ImU32 labelColor, double& value,
               float fullWidth) {
    ImGui::PushID(label);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float h = 34.0f;
    ImDrawList* d = ImGui::GetWindowDrawList();
    d->AddRectFilled(p, plus(p, fullWidth, h), kInput32, 4);
    d->AddRect(p, plus(p, fullWidth, h), kBorder32, 4);
    smallText(d, plus(p, 10, 10), labelColor, label, 12);
    ImGui::SetCursorScreenPos(plus(p, 29, 2));
    ImGui::SetNextItemWidth(std::max(20.0f, fullWidth - 37));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
    const bool changed = ImGui::InputDouble("##numeric", &value, 0, 0, "%.3f");
    ImGui::PopStyleColor(3);
    ImGui::SetCursorScreenPos({p.x, p.y + h + 7});
    ImGui::PopID();
    return changed;
}

void drawCameraInspector(Session& session, FullscreenEditorState& ui, Rect rect) {
    ImGui::TextColored(kMuted, "VIRTUAL CAMERA POSE");
    ImGui::TextColored(kMuted, "Edit coordinates to prepare a shot.");
    ImGui::TextDisabled("Not affecting the game.");
    ImGui::Dummy({0, 9});
    const float width = std::max(90.0f, rect.width - 56);
    CameraState pose = session.rig().pose();
    bool changed = false;
    changed |= axisInput("X", kRed32, pose.position.x, width);
    changed |= axisInput("Y", kGreen32, pose.position.y, width);
    changed |= axisInput("Z", kBlue32, pose.position.z, width);
    changed |= axisInput("FOV", kAccent32, pose.fovDegrees, width);
    if (changed && !session.setCameraPose(pose))
        ui.status = "Invalid virtual camera position/FOV.";

    ImGui::Dummy({0, 5});
    ImGui::Separator();
    ImGui::Dummy({0, 4});
    ImGui::TextColored(kMuted, "KEYFRAMES");
    ImGui::Dummy({0, 4});
    (void)axisInput("T", kMuted32, ui.addTimeSeconds, width);
    ImGui::SetCursorPosX(0);
    if (primaryButton(withIcon(fa::add, "Add virtual pose").c_str(), {width, 30})) {
        addMessage(ui, session.addKeyframe(ui.addTimeSeconds)
            ? "Virtual keyframe added." : "Invalid or duplicate keyframe time.");
    }
    ImGui::Dummy({0, 5});
    if (session.path().size() == 0) {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        dashedRect(ImGui::GetWindowDrawList(), p, plus(p, width, 75), kBorder32);
        centeredText(ImGui::GetWindowDrawList(), plus(p, width * .5f, 36),
                     kMuted32, "No points yet. Add a virtual pose.", 11);
        ImGui::Dummy({width, 79});
    } else {
        for (std::size_t i = 0; i < session.path().size(); ++i) {
            const Keyframe frame = session.path().keyframes()[i];
            ImGui::PushID(static_cast<int>(i));
            ImGui::Text("%zu  |  %.3fs", i, frame.timeSeconds);
            ImGui::SameLine();
            if (ImGui::SmallButton(withIcon(fa::play, "Go").c_str())) session.seek(frame.timeSeconds);
            ImGui::SameLine();
            if (ImGui::SmallButton(iconsLoaded() ? fa::trash : "X")) {
                (void)session.removeKeyframe(i);
                addMessage(ui, "Keyframe removed.");
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
    }
}

void drawConsole(Session& session, FullscreenEditorState& ui) {
    const float inputHeight = ImGui::GetFrameHeightWithSpacing() + 12.0f;
    const float logsHeight = std::max(48.0f, ImGui::GetContentRegionAvail().y - inputHeight);
    ImGui::BeginChild("##NCRECConsoleLog", ImVec2(0, logsHeight),
                      ImGuiChildFlags_Borders);
    for (const std::string& line : ui.consoleLines) ImGui::TextUnformatted(line.c_str());
    ImGui::EndChild();
    if (ui.focusConsole) {
        ImGui::SetKeyboardFocusHere();
        ui.focusConsole = false;
    }
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputTextWithHint("##NCRECCommand",
            "Type a command then press Enter...", ui.consoleInput,
            sizeof(ui.consoleInput), ImGuiInputTextFlags_EnterReturnsTrue)) {
        runCommand(session, ui, ui.consoleInput);
        ui.consoleInput[0] = '\0';
        ImGui::SetKeyboardFocusHere(-1);
    }
}

void drawInspector(Session& session, FullscreenEditorState& ui, Rect rect) {
    if (!beginPanel("##NCRECInspector", rect, kPanel)) return;
    panelHeading(rect, "INSPECTOR", fa::settings);
    const float availableWidth = rect.width - 32;
    const float tabWidth = std::max(58.0f, (availableWidth - 16) / 3.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, kMain);
    const char* tabs[] = {"Timeline", "Console", "Settings"};
    const char* tabIcons[] = {fa::timeline, fa::terminal, fa::settings};
    for (int n = 0; n < 3; ++n) {
        if (n) ImGui::SameLine(0, 4);
        const bool selected = static_cast<int>(ui.inspector) == n;
        if (selected) ImGui::PushStyleColor(ImGuiCol_Button, kInput);
        if (selected) ImGui::PushStyleColor(ImGuiCol_Text, kAccent);
        if (ImGui::Button(withIcon(tabIcons[n], tabs[n]).c_str(), {tabWidth, 29}))
            ui.inspector = static_cast<InspectorTab>(n);
        if (selected) ImGui::PopStyleColor(2);
    }
    ImGui::PopStyleColor();
    ImGui::Dummy({0, 14});

    const ImGuiWindowFlags contentFlags =
        ui.inspector == InspectorTab::Console
            ? ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
            : ImGuiWindowFlags_None;
    ImGui::BeginChild("##InspectorContent", {availableWidth, 0},
                      ImGuiChildFlags_None, contentFlags);
    switch (ui.inspector) {
        case InspectorTab::Camera:
            drawCameraInspector(session, ui, rect);
            break;
        case InspectorTab::Console:
            drawConsole(session, ui);
            break;
        case InspectorTab::Settings:
            ImGui::TextColored(kMuted, "APPEARANCE");
            ImGui::SliderFloat("UI scale", &ui.uiScale, .75f, 1.75f, "%.2fx");
            ImGui::TextWrapped("Input ownership and native game camera access "
                               "are still disabled in this visual preview.");
            break;
    }
    ImGui::EndChild();
    endPanel();
}

void drawTimeline(Session& session, FullscreenEditorState& ui, Rect rect) {
    if (!beginPanel("##NCRECTimeline", rect, kPanel)) return;
    panelHeading(rect, "CAMERA TIMELINE", fa::timeline);

    ImGui::TextColored(kMuted, "Build and preview timed camera shots.");
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const float width = rect.width - 32;
    const float duration = static_cast<float>(session.path().durationSeconds());
    const float range = std::max(duration, 8.0f);

    const ImVec2 bar = plus(start, 0, 18);
    const ImVec2 barBottom = plus(bar, width, 48);
    ImDrawList* d = ImGui::GetWindowDrawList();
    d->AddRectFilled(bar, barBottom, kInput32, 4);
    d->AddRect(bar, barBottom, kBorder32, 4);

    for (int tick = 0; tick <= 8; ++tick) {
        const float f = static_cast<float>(tick) / 8.0f;
        const float x = bar.x + (barBottom.x - bar.x) * f;
        d->AddLine({x, bar.y + 16}, {x, bar.y + 32},
                   IM_COL32(100, 100, 100, 255));
        char label[24]{};
        std::snprintf(label, sizeof(label), "%.0fs", range * f);
        smallText(d, {x, barBottom.y + 10}, kMuted32, label, 11);
    }

    for (const Keyframe& k : session.path().keyframes()) {
        const float x = bar.x + (barBottom.x - bar.x) *
            static_cast<float>(k.timeSeconds / range);
        d->AddCircleFilled({x, bar.y + 24}, 5, kAccent32);
    }
    const float cursorTime = static_cast<float>(session.cursorSeconds());
    const float cursorX = bar.x + (barBottom.x - bar.x) * cursorTime / range;
    d->AddLine({cursorX, bar.y - 5}, {cursorX, barBottom.y + 5}, kAccent32, 2);
    d->AddCircleFilled({cursorX, bar.y - 5}, 5, kAccent32);

    ImGui::SetCursorScreenPos(bar);
    ImGui::InvisibleButton("##TimeScrub", {width, 48});
    if (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        const float ratio = std::clamp((ImGui::GetIO().MousePos.x - bar.x) /
            std::max(width, 1.0f), 0.0f, 1.0f);
        session.seek(ratio * duration);
    }

    const ImVec2 line = {rect.x + 16, barBottom.y + 41};
    char summary[160]{};
    std::snprintf(summary, sizeof(summary), "%zu points  /  %.3f seconds     Timeline %.3fs",
                  session.path().size(), duration, session.cursorSeconds());
    smallText(d, line, kMuted32, summary, 12);
    if (!ui.status.empty())
        smallText(d, plus(line, 0, 23), kMuted32, ui.status.c_str(), 11);
    endPanel();
}

void drawStatus(Session& session, Rect rect) {
    if (!beginPanel("##NCRECStatus", rect, kHeader)) return;
    ImDrawList* d = ImGui::GetWindowDrawList();
    d->AddCircleFilled({rect.x + 16, rect.y + 13}, 4, IM_COL32(85, 220, 85, 255));
    smallText(d, {rect.x + 26, rect.y + 6}, kText32,
              "Ready  |  Virtual Camera: GAME NOT CONNECTED", 11);
    char right[90]{};
    std::snprintf(right, sizeof(right), "NCREC  /  %zu keyframes  /  %.3fs",
                  session.path().size(), session.cursorSeconds());
    const ImVec2 extent = ImGui::GetFont()->CalcTextSizeA(11, 10000, 0, right);
    smallText(d, {rect.x + std::max(0.0f, rect.width - extent.x - 14), rect.y + 6},
              kMuted32, right, 11);
    endPanel();
}

} // namespace

void drawFullscreenEditor(Session& session, FullscreenEditorState& ui) {
    // Render-only until native game cursor ownership is verified.
    if (!ImGui::GetCurrentContext()) return;
    const ImGuiIO& io = ImGui::GetIO();
    const WorkspaceLayout layout =
        calculateWorkspace(io.DisplaySize.x, io.DisplaySize.y, ui.uiScale);
    if (layout.screen.width <= 0 || layout.screen.height <= 0) return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(9, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(7, 7));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, kMain);
    ImGui::PushStyleColor(ImGuiCol_Border, kPanel);
    ImGui::PushStyleColor(ImGuiCol_Button, kInput);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.23f, .23f, .23f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.30f, .30f, .30f, 1));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, kInput);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(.20f, .20f, .20f, 1));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(.26f, .26f, .26f, 1));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(.88f, .88f, .88f, 1));

    ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({layout.screen.width, layout.screen.height}, ImGuiCond_Always);
    if (ImGui::Begin("##NCRECFullscreenWorkspace", nullptr,
                     kFixedFlags | ImGuiWindowFlags_NoBringToFrontOnFocus)) {
        drawTopBar(session, ui, layout.menuBar);
        if (layout.showProjectPanel) drawProjects(session, ui, layout.projectPanel);
        drawPreview(layout.preview);
        drawPlayback(session, ui, layout.playback);
        if (layout.showInspector) drawInspector(session, ui, layout.inspector);
        drawTimeline(session, ui, layout.timeline);
        drawStatus(session, layout.statusBar);
    }
    ImGui::End();
    ImGui::PopStyleColor(9);
    ImGui::PopStyleVar(6);
}

} // namespace ncrec::editor
