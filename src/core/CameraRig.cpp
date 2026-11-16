#include "core/CameraRig.hpp"

#include <algorithm>
#include <cmath>

namespace ncrec {
namespace {

constexpr double kPi = 3.1415926535897932384626433832795;

bool finite(double value) { return std::isfinite(value); }

Quaternion normalized(Quaternion q) {
    const double m = std::hypot(std::hypot(q.x, q.y), std::hypot(q.z, q.w));
    return {q.x/m, q.y/m, q.z/m, q.w/m};
}

Quaternion multiply(Quaternion a, Quaternion b) {
    return {
        a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
        a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
        a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w,
        a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z
    };
}

Quaternion axisAngle(Vec3 axis, double degrees) {
    const double half = degrees * (kPi / 360.0);
    const double sine = std::sin(half);
    return {axis.x * sine, axis.y * sine, axis.z * sine, std::cos(half)};
}

Vec3 rotated(Quaternion q, Vec3 v) {
    const Quaternion p = multiply(multiply(q, {v.x, v.y, v.z, 0.0}),
                                  {-q.x, -q.y, -q.z, q.w});
    return {p.x, p.y, p.z};
}

bool valid(const CameraState& p) {
    const double squared = p.rotation.x*p.rotation.x + p.rotation.y*p.rotation.y
        + p.rotation.z*p.rotation.z + p.rotation.w*p.rotation.w;
    return finite(p.position.x) && finite(p.position.y) && finite(p.position.z)
        && finite(p.rotation.x) && finite(p.rotation.y)
        && finite(p.rotation.z) && finite(p.rotation.w)
        && finite(squared) && squared > 1e-20
        && finite(p.fovDegrees) && p.fovDegrees > 0.0 && p.fovDegrees < 180.0;
}

} // namespace

bool CameraRig::setPose(const CameraState& state) noexcept {
    if (!valid(state)) return false;
    pose_ = state;
    pose_.rotation = normalized(state.rotation);
    return true;
}

bool CameraRig::step(const CameraMotion& motion, double dtSeconds,
                     double unitsPerSecond) noexcept {
    if (!finite(dtSeconds) || dtSeconds < 0 || dtSeconds > 1.0
        || !finite(unitsPerSecond) || unitsPerSecond < 0 || unitsPerSecond > 10000
        || !finite(motion.right) || !finite(motion.forward) || !finite(motion.up)
        || !finite(motion.yawDegrees) || !finite(motion.pitchDegrees)
        || !finite(motion.rollDegrees)) return false;

    // Fixed delta behavior is intentional: the UI can request precise
    // spatial or angular nudges without using wall-clock frame timing.
    const Quaternion q = pose_.rotation;
    const Vec3 localMovement{
        std::clamp(motion.right, -1.0, 1.0),
        std::clamp(motion.forward, -1.0, 1.0),
        std::clamp(motion.up, -1.0, 1.0)
    };
    const double norm = std::hypot(std::hypot(localMovement.x, localMovement.y),
                                   localMovement.z);
    const double magnitude = norm > 1 ? 1 / norm : 1.0;
    const Vec3 world = rotated(q, localMovement);
    CameraState next = pose_;
    next.position.x += world.x * magnitude * unitsPerSecond * dtSeconds;
    next.position.y += world.y * magnitude * unitsPerSecond * dtSeconds;
    next.position.z += world.z * magnitude * unitsPerSecond * dtSeconds;

    // Yaw is about the world's upright axis; pitch/roll are camera-local.
    const Quaternion yaw = axisAngle({0,0,1}, motion.yawDegrees * dtSeconds);
    const Quaternion pitch = axisAngle({1,0,0}, motion.pitchDegrees * dtSeconds);
    const Quaternion roll = axisAngle({0,1,0}, motion.rollDegrees * dtSeconds);
    next.rotation = normalized(multiply(multiply(multiply(yaw, q), pitch), roll));
    return setPose(next);
}

} // namespace ncrec
