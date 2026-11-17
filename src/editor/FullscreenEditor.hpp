#pragma once

#include <string>
#include <vector>

namespace ncrec {
class Session;
}

namespace ncrec::editor {

// This module only draws into an existing ImGui frame. It never installs DXGI
// hooks, creates Win32 windows or acquires game input. RED4ext must supply a
// validated renderer/input lifecycle before calling it.
enum class InspectorTab { Camera, Console, Settings };

struct FullscreenEditorState {
    char shotName[65]{"shot_001"};
    char consoleInput[512]{};
    double addTimeSeconds{};
    float uiScale{1.0f};
    InspectorTab inspector{InspectorTab::Camera};
    std::string status{"Editor preview - no game camera connection"};
    std::vector<std::string> consoleLines;
    bool closeRequested{};
    bool focusConsole{};
};

void drawFullscreenEditor(Session& session, FullscreenEditorState& state);

} // namespace ncrec::editor
