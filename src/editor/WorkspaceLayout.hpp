#pragma once

namespace ncrec::editor {

// Pixel geometry for the fullscreen workspace in demo.html, independent of
// ImGui/D3D. All regions fit inside the current game framebuffer.
struct Rect {
    float x{};
    float y{};
    float width{};
    float height{};
};

struct WorkspaceLayout {
    Rect screen{};
    Rect menuBar{};
    Rect projectPanel{};
    Rect preview{};
    Rect playback{};
    Rect inspector{};
    Rect timeline{};
    Rect statusBar{};

    bool showProjectPanel{};
    bool showInspector{};
};

// Calculates a non-scrolling fullscreen shell. At small resolutions the
// optional sidebars collapse instead of squeezing the preview to zero.
[[nodiscard]] WorkspaceLayout calculateWorkspace(
    float framebufferWidth, float framebufferHeight, float uiScale = 1.0f) noexcept;

} // namespace ncrec::editor
