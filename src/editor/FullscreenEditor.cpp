#include "editor/FullscreenEditor.hpp"

#include "editor/WorkspaceLayout.hpp"
#include "core/Session.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace ncrec::editor {
namespace {

const ImVec4 kBackground{0.059f, 0.059f, 0.059f, 1.0f};
const ImVec4 kPanel{0.102f, 0.102f, 0.102f, 1.0f};
const ImVec4 kHeader{0.078f, 0.078f, 0.078f, 1.0f};
const ImVec4 kBorder{0.20f, 0.20f, 0.20f, 1.0f};
const ImVec4 kMuted{0.54f, 0.54f, 0.54f, 1.0f};
const ImVec4 kYellow{0.99f, 0.93f, 0.039f, 1.0f};

constexpr ImGuiWindowFlags kFixedFlags =
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
    ImGuiWindowFlags_NoSavedSettings;

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

bool primaryButton(const char* label, const ImVec2& size = ImVec2(0, 0)) {
    ImGui::PushStyleColor(ImGuiCol_Button, kYellow);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.98f, 0.30f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.86f, 0.80f, 0.04f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.04f, 0.04f, 0.04f, 1.0f));
    const bool clicked = ImGui::Button(label, size);
    ImGui::PopStyleColor(4);
    return clicked;
}

bool beginPanel(const char* id, Rect rect, ImVec4 background, bool scroll = false) {
    if (rect.width < 1.0f || rect.height < 1.0f) return false;
    ImGui::SetCursorScreenPos(ImVec2(rect.x, rect.y));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 9.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, background);
    const ImGuiWindowFlags flags = scroll ? ImGuiWindowFlags_NoSavedSettings : kFixedFlags;
    ImGui::BeginChild(id, ImVec2(rect.width, rect.height), ImGuiChildFlags_Borders, flags);
    return true;
}

void endPanel() {
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void heading(const char* name) {
    ImGui::TextColored(kMuted, "%s", name);
    ImGui::Separator();
}

void drawTopBar(Session& session, FullscreenEditorState& ui, Rect rect) {
    if (!beginPanel("##NCRECTopBar", rect, kHeader)) return;
    ImGui::TextColored(kYellow, "NCREC");
    ImGui::SameLine(88);
    if (ImGui::Button("File")) ImGui::OpenPopup("NCRECFile");
    if (ImGui::BeginPopup("NCRECFile")) {
        if (ImGui::MenuItem("New shot")) {
            runCommand(session, ui, "path clear");
        }
        if (ImGui::MenuItem("Save shot")) saveShot(session, ui);
        if (ImGui::MenuItem("Load shot")) loadShot(session, ui);
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Playback")) {
        runCommand(session, ui, session.playing() ? "path pause" : "path play");
    }
    ImGui::SameLine();
    if (ImGui::Button("Console")) ui.inspector = InspectorTab::Console;
    ImGui::SameLine();
    if (ImGui::Button("Settings")) ui.inspector = InspectorTab::Settings;
    ImGui::SameLine();
    if (ImGui::Button("Hide NCREC")) ui.closeRequested = true;
    endPanel();
}

void drawProjects(Session& session, FullscreenEditorState& ui, Rect rect) {
    if (!beginPanel("##NCRECProjects", rect, kPanel)) return;
    heading("PROJECT / SHOTS");
    ImGui::TextColored(kMuted, "Current project");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##shotName", "shot_001", ui.shotName, sizeof(ui.shotName));
    if (primaryButton("New shot", ImVec2(-1, 0))) {
        runCommand(session, ui, "path clear");
    }
    if (ImGui::Button("Save shot", ImVec2(-1, 0))) saveShot(session, ui);
    if (ImGui::Button("Load shot", ImVec2(-1, 0))) loadShot(session, ui);
    ImGui::TextColored(kMuted, "%zu keyframes in current shot", session.path().size());
    endPanel();
}

void drawPreview(Rect rect) {
    if (!beginPanel("##NCRECPreview", rect, ImVec4(0.020f, 0.020f, 0.020f, 1.0f))) return;
    heading("CAMERA PREVIEW");
    ImGui::TextColored(kMuted, "SELECT  /  PAN  /  ORBIT  /  ZOOM");
    const ImVec2 size = ImGui::GetContentRegionAvail();
    if (size.y > 30.0f) ImGui::Dummy(ImVec2(1.0f, size.y * 0.36f));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 16.0f);
    ImGui::TextDisabled("[ Camera preview unavailable ]");
    ImGui::TextColored(kMuted,
        "RED4ext renderer and active game camera integration are not enabled.");
    endPanel();
}

void drawPlayback(Session& session, FullscreenEditorState& ui, Rect rect) {
    if (!beginPanel("##NCRECPlayback", rect, kHeader)) return;
    if (ImGui::Button("Load demo")) runCommand(session, ui, "path demo");
    ImGui::SameLine();
    if (ImGui::Button("Rewind")) session.seek(0.0);
    ImGui::SameLine();
    if (primaryButton(session.playing() ? "Pause" : "Play")) {
        runCommand(session, ui, session.playing() ? "path pause" : "path play");
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear")) runCommand(session, ui, "path clear");
    endPanel();
}

void drawCameraInspector(Session& session, FullscreenEditorState& ui) {
    ImGui::TextColored(kMuted, "VIRTUAL CAMERA POSE");
    ImGui::TextDisabled("These values do not affect Cyberpunk's viewpoint.");
    CameraState pose = session.rig().pose();
    ImGui::SetNextItemWidth(-1);
    bool change = ImGui::InputDouble("X##pose", &pose.position.x, 0.1, 1.0, "%.3f");
    ImGui::SetNextItemWidth(-1);
    change |= ImGui::InputDouble("Y##pose", &pose.position.y, 0.1, 1.0, "%.3f");
    ImGui::SetNextItemWidth(-1);
    change |= ImGui::InputDouble("Z##pose", &pose.position.z, 0.1, 1.0, "%.3f");
    ImGui::SetNextItemWidth(-1);
    change |= ImGui::InputDouble("FOV##pose", &pose.fovDegrees, 1.0, 5.0, "%.1f");
    if (change && !session.setCameraPose(pose)) {
        ui.status = "Invalid camera coordinates or field of view.";
    }
    ImGui::Separator();
    ImGui::TextColored(kMuted, "KEYFRAMES");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("Time (s)##keyframe", &ui.addTimeSeconds, 0.25, 1.0, "%.3f");
    if (primaryButton("Add virtual pose", ImVec2(-1, 0))) {
        addMessage(ui, session.addKeyframe(ui.addTimeSeconds)
            ? "Virtual keyframe added." : "Invalid or duplicate keyframe time.");
    }
    ImGui::Separator();
    // The inspector body is the only panel allowed to scroll when there are
    // many keyframes. The application shell and inspector root never scroll.
    for (std::size_t i = 0; i < session.path().size(); ++i) {
        const Keyframe frame = session.path().keyframes()[i];
        ImGui::PushID(static_cast<int>(i));
        ImGui::Text("%zu  @ %.3fs", i, frame.timeSeconds);
        ImGui::SameLine();
        if (ImGui::SmallButton("Go")) session.seek(frame.timeSeconds);
        ImGui::SameLine();
        if (ImGui::SmallButton("Remove")) {
            (void)session.removeKeyframe(i);
            addMessage(ui, "Keyframe deleted.");
            ImGui::PopID();
            break;
        }
        ImGui::PopID();
    }
}

void drawConsole(Session& session, FullscreenEditorState& ui) {
    const float inputHeight = ImGui::GetFrameHeightWithSpacing() + 12.0f;
    const float logsHeight = std::max(48.0f, ImGui::GetContentRegionAvail().y - inputHeight);
    ImGui::BeginChild("##NCRECConsoleLog", ImVec2(0, logsHeight), ImGuiChildFlags_Borders);
    for (const std::string& line : ui.consoleLines) {
        ImGui::TextUnformatted(line.c_str());
    }
    ImGui::EndChild();
    if (ui.focusConsole) {
        ImGui::SetKeyboardFocusHere();
        ui.focusConsole = false;
    }
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputTextWithHint("##NCRECCommand", "Type a command then press Enter...",
            ui.consoleInput, sizeof(ui.consoleInput),
            ImGuiInputTextFlags_EnterReturnsTrue)) {
        runCommand(session, ui, ui.consoleInput);
        ui.consoleInput[0] = '\0';
        ImGui::SetKeyboardFocusHere(-1);
    }
}

void drawInspector(Session& session, FullscreenEditorState& ui, Rect rect) {
    if (!beginPanel("##NCRECInspector", rect, kPanel)) return;
    heading("INSPECTOR");
    const char* tabs[] = {"Camera", "Console", "Settings"};
    for (int n = 0; n < 3; ++n) {
        if (n) ImGui::SameLine();
        const bool selected = static_cast<int>(ui.inspector) == n;
        if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.27f,.25f,.08f,1));
        if (ImGui::Button(tabs[n])) ui.inspector = static_cast<InspectorTab>(n);
        if (selected) ImGui::PopStyleColor();
    }
    ImGui::Separator();
    // Only one region can scroll at a time. Camera/Settings scroll this
    // content area; Console scrolls only its log child (not both layers).
    const ImGuiWindowFlags contentFlags =
        ui.inspector == InspectorTab::Console
            ? ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
            : ImGuiWindowFlags_None;
    ImGui::BeginChild("##InspectorContent", ImVec2(0, 0),
                      ImGuiChildFlags_None, contentFlags);
    switch (ui.inspector) {
        case InspectorTab::Camera: drawCameraInspector(session, ui); break;
        case InspectorTab::Console: drawConsole(session, ui); break;
        case InspectorTab::Settings:
            ImGui::TextColored(kMuted, "APPEARANCE");
            ImGui::SliderFloat("UI scale", &ui.uiScale, 0.75f, 1.75f, "%.2fx");
            ImGui::TextWrapped("The standalone RED4ext renderer and input settings "
                               "will be available once their game compatibility is verified.");
            break;
    }
    ImGui::EndChild();
    endPanel();
}

void drawTimeline(Session& session, FullscreenEditorState& ui, Rect rect) {
    if (!beginPanel("##NCRECTimeline", rect, kPanel)) return;
    heading("CAMERA TIMELINE");
    ImGui::TextColored(kMuted, "Offline camera path preview - time in seconds");
    const float time = static_cast<float>(session.cursorSeconds());
    const float duration = static_cast<float>(session.path().durationSeconds());
    const float maxTime = std::max(duration, 0.01f);
    const float width = std::max(1.0f, ImGui::GetContentRegionAvail().x);
    const float trackHeight = std::min(72.0f, std::max(20.0f, ImGui::GetContentRegionAvail().y - 12));
    ImGui::InvisibleButton("##TimelineTrack", ImVec2(width, trackHeight));
    const ImVec2 start = ImGui::GetItemRectMin();
    const ImVec2 end = ImGui::GetItemRectMax();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const float lineY = start.y + 32.0f;
    draw->AddLine(ImVec2(start.x, lineY), ImVec2(end.x, lineY), IM_COL32(82,82,82,255), 2.0f);

    for (int i = 0; i <= 10; ++i) {
        const float x = start.x + (end.x-start.x) * (static_cast<float>(i)/10.0f);
        draw->AddLine(ImVec2(x,lineY-8), ImVec2(x,lineY+8), IM_COL32(90,90,90,255));
    }
    for (const Keyframe& k : session.path().keyframes()) {
        const float x = start.x + (end.x-start.x) *
            static_cast<float>(k.timeSeconds / maxTime);
        draw->AddCircleFilled(ImVec2(x,lineY), 5.0f, IM_COL32(252,238,10,255));
    }
    const float cursorX = start.x + (end.x-start.x) * time / maxTime;
    draw->AddLine(ImVec2(cursorX,start.y + 7), ImVec2(cursorX,end.y - 5),
                  IM_COL32(252,238,10,255), 2.0f);

    if (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        const float fraction = std::clamp((ImGui::GetIO().MousePos.x - start.x) /
                            std::max(1.0f, end.x - start.x), 0.0f, 1.0f);
        session.seek(fraction * duration);
    }
    ImGui::TextColored(kMuted, "%zu points    %.3f s    cursor %.3f s",
                       session.path().size(), duration, session.cursorSeconds());
    if (!ui.status.empty()) ImGui::TextColored(kMuted, "%s", ui.status.c_str());
    endPanel();
}

void drawStatus(Session& session, Rect rect) {
    if (!beginPanel("##NCRECStatus", rect, kHeader)) return;
    ImGui::TextColored(kMuted, "NCREC  |  RED4ext editor prototype  |  GAME CAMERA DISCONNECTED");
    ImGui::SameLine();
    ImGui::TextColored(kMuted, "   %zu keyframes  %.3fs",
                       session.path().size(), session.cursorSeconds());
    endPanel();
}

} // namespace

void drawFullscreenEditor(Session& session, FullscreenEditorState& ui) {
    // This function is intentionally never called by the loader-only RED4ext
    // plugin. The future host must initialize and own ImGui/render/input.
    if (!ImGui::GetCurrentContext()) return;
    const ImGuiIO& io = ImGui::GetIO();
    const WorkspaceLayout layout =
        calculateWorkspace(io.DisplaySize.x, io.DisplaySize.y, ui.uiScale);
    if (layout.screen.width <= 0.0f || layout.screen.height <= 0.0f) return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, kBackground);
    ImGui::PushStyleColor(ImGuiCol_Border, kBorder);
    ImGui::SetNextWindowPos(ImVec2(0,0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(layout.screen.width,layout.screen.height), ImGuiCond_Always);
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
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

} // namespace ncrec::editor
