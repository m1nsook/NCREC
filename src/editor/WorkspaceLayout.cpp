#include "editor/WorkspaceLayout.hpp"

#include <algorithm>
#include <cmath>

namespace ncrec::editor {

WorkspaceLayout calculateWorkspace(
    float framebufferWidth, float framebufferHeight, float uiScale) noexcept {

    WorkspaceLayout out{};
    if (!std::isfinite(framebufferWidth) || !std::isfinite(framebufferHeight)
        || framebufferWidth <= 0.0f || framebufferHeight <= 0.0f) {
        return out;
    }
    if (!std::isfinite(uiScale)) uiScale = 1.0f;

    // The values match the HTML prototype at 100% scaling:
    // 36px top bar, 26px status, 240px timeline, 260/320px sidebars,
    // and 44px transport under the center viewport.
    const float s = std::clamp(uiScale, 0.75f, 2.0f);
    const float w = framebufferWidth;
    const float h = framebufferHeight;
    out.screen = {0, 0, w, h};

    const float top = std::min(h, 36.0f * s);
    const float status = std::min(h - top, 26.0f * s);
    const float remaining = h - top - status;
    const float timeline = std::min(240.0f * s, remaining * 0.38f);
    const float mainHeight = remaining - timeline;
    const float transport = std::min(44.0f * s, mainHeight);

    out.menuBar = {0, 0, w, top};
    out.timeline = {0, top + mainHeight, w, timeline};
    out.statusBar = {0, h - status, w, status};

    // The screenshot target is an editing application, not a floating
    // overlay. Keep a usable preview on smaller resolutions.
    const float minimumPreview = 420.0f * s;
    const float leftWanted = 260.0f * s;
    const float rightWanted = 320.0f * s;
    const bool both = w >= minimumPreview + leftWanted + rightWanted;
    const bool inspectorOnly = !both && w >= minimumPreview + rightWanted;
    out.showProjectPanel = both;
    out.showInspector = both || inspectorOnly;
    const float left = both ? leftWanted : 0.0f;
    const float right = out.showInspector ? rightWanted : 0.0f;
    const float center = std::max(0.0f, w - left - right);

    out.projectPanel = {0, top, left, mainHeight};
    out.preview = {left, top, center, mainHeight - transport};
    out.playback = {left, top + mainHeight - transport, center, transport};
    out.inspector = {left + center, top, right, mainHeight};

    return out;
}

} // namespace ncrec::editor
