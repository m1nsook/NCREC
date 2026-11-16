#pragma once

#include "core/CameraPath.hpp"

namespace ncrec {

// NCREC's own coordinate convention: world Z is up, camera-local Y is
// forward, X is right. The future REDengine adapter converts conventions.
struct CameraMotion {
    double right{};
    double forward{};
    double up{};
    double yawDegrees{};
    double pitchDegrees{};
    double rollDegrees{};
};

class CameraRig {
public:
    [[nodiscard]] const CameraState& pose() const noexcept { return pose_; }
    [[nodiscard]] bool setPose(const CameraState& state) noexcept;
    [[nodiscard]] bool step(const CameraMotion& motion, double dtSeconds,
                            double unitsPerSecond = 5.0) noexcept;
private:
    CameraState pose_{};
};

} // namespace ncrec
