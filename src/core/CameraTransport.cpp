#include "core/CameraTransport.hpp"

#include <cmath>

namespace ncrec {
namespace {
bool validCamera(const CameraState& c) {
    const double squared = c.rotation.x*c.rotation.x + c.rotation.y*c.rotation.y
        + c.rotation.z*c.rotation.z + c.rotation.w*c.rotation.w;
    return std::isfinite(c.position.x) && std::isfinite(c.position.y)
        && std::isfinite(c.position.z) && std::isfinite(c.fovDegrees)
        && c.fovDegrees > 0 && c.fovDegrees < 180
        && std::isfinite(c.rotation.x) && std::isfinite(c.rotation.y)
        && std::isfinite(c.rotation.z) && std::isfinite(c.rotation.w)
        && std::isfinite(squared) && squared > 1e-20;
}
}
bool CameraTransport::start() noexcept {
    if (active_) return true;
    if (!adapter_.isSupported()) return false;
    CameraState initial{};
    if (!adapter_.read(initial) || !validCamera(initial)) return false;
    if (!adapter_.beginOverride()) return false;
    active_ = true;
    return true;
}
bool CameraTransport::apply(const CameraState& pose) noexcept {
    if (!active_ || !validCamera(pose)) return false;
    if (adapter_.apply(pose)) return true;
    stop(); // fail closed: game camera ownership always released on error.
    return false;
}
void CameraTransport::stop() noexcept {
    if (!active_) return;
    active_ = false;
    adapter_.endOverride();
}
} // namespace ncrec
