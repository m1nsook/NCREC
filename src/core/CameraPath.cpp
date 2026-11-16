#include "core/CameraPath.hpp"

#include <algorithm>
#include <cmath>

namespace ncrec {
namespace {

constexpr double kDuplicateTimeTolerance = 1e-9;

[[nodiscard]] bool finite(double value) {
    return std::isfinite(value);
}

[[nodiscard]] bool validCamera(const CameraState& state) {
    const auto& p = state.position;
    const auto& q = state.rotation;
    return finite(p.x) && finite(p.y) && finite(p.z)
        && finite(q.x) && finite(q.y) && finite(q.z) && finite(q.w)
        && (q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w > 1e-20)
        && finite(state.fovDegrees) && state.fovDegrees > 0.0 && state.fovDegrees < 180.0;
}

[[nodiscard]] Vec3 add(Vec3 a, Vec3 b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

[[nodiscard]] Vec3 subtract(Vec3 a, Vec3 b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

[[nodiscard]] Vec3 scale(Vec3 v, double s) {
    return {v.x * s, v.y * s, v.z * s};
}

[[nodiscard]] Vec3 lerp(Vec3 a, Vec3 b, double t) {
    return add(scale(a, 1.0 - t), scale(b, t));
}

[[nodiscard]] Quaternion normalized(Quaternion q) {
    const double length = std::sqrt(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
    return {q.x / length, q.y / length, q.z / length, q.w / length};
}

[[nodiscard]] Quaternion slerp(Quaternion from, Quaternion to, double t) {
    from = normalized(from);
    to = normalized(to);

    double dot = from.x*to.x + from.y*to.y + from.z*to.z + from.w*to.w;
    if (dot < 0.0) {
        to = {-to.x, -to.y, -to.z, -to.w};
        dot = -dot;
    }

    dot = std::clamp(dot, -1.0, 1.0);
    double firstWeight = 1.0 - t;
    double secondWeight = t;
    if (dot < 0.9995) {
        const double angle = std::acos(dot);
        const double denominator = std::sin(angle);
        firstWeight = std::sin((1.0 - t) * angle) / denominator;
        secondWeight = std::sin(t * angle) / denominator;
    }

    return normalized({
        from.x * firstWeight + to.x * secondWeight,
        from.y * firstWeight + to.y * secondWeight,
        from.z * firstWeight + to.z * secondWeight,
        from.w * firstWeight + to.w * secondWeight
    });
}

// Time-aware cubic Hermite interpolation. Two points reduce to a straight segment.
[[nodiscard]] Vec3 cubicPosition(const std::vector<Keyframe>& frames,
                                 std::size_t left, double t) {
    const std::size_t right = left + 1;
    const auto& a = frames[left];
    const auto& b = frames[right];
    const double span = b.timeSeconds - a.timeSeconds;

    const Vec3 segmentSlope = scale(subtract(b.camera.position, a.camera.position), 1.0 / span);
    const Vec3 startSlope = left == 0
        ? segmentSlope
        : scale(subtract(b.camera.position, frames[left - 1].camera.position),
                1.0 / (b.timeSeconds - frames[left - 1].timeSeconds));
    const Vec3 endSlope = right + 1 == frames.size()
        ? segmentSlope
        : scale(subtract(frames[right + 1].camera.position, a.camera.position),
                1.0 / (frames[right + 1].timeSeconds - a.timeSeconds));

    const double t2 = t * t;
    const double t3 = t2 * t;
    const double h00 = 2.0*t3 - 3.0*t2 + 1.0;
    const double h10 = t3 - 2.0*t2 + t;
    const double h01 = -2.0*t3 + 3.0*t2;
    const double h11 = t3 - t2;

    return add(
        add(scale(a.camera.position, h00), scale(startSlope, h10 * span)),
        add(scale(b.camera.position, h01), scale(endSlope, h11 * span))
    );
}

[[nodiscard]] CameraState canonical(CameraState state) {
    state.rotation = normalized(state.rotation);
    return state;
}

} // namespace

bool CameraPath::addKeyframe(const Keyframe& frame) {
    if (!finite(frame.timeSeconds) || frame.timeSeconds < 0.0 || !validCamera(frame.camera)) {
        return false;
    }
    auto position = std::lower_bound(
        keyframes_.begin(), keyframes_.end(), frame.timeSeconds,
        [](const Keyframe& keyframe, double time) { return keyframe.timeSeconds < time; });

    if ((position != keyframes_.end()
            && std::abs(position->timeSeconds - frame.timeSeconds) <= kDuplicateTimeTolerance)
        || (position != keyframes_.begin()
            && std::abs((position - 1)->timeSeconds - frame.timeSeconds) <= kDuplicateTimeTolerance)) {
        return false;
    }

    keyframes_.insert(position, {frame.timeSeconds, canonical(frame.camera)});
    return true;
}

void CameraPath::clear() noexcept {
    keyframes_.clear();
}

void CameraPath::setPositionInterpolation(PositionInterpolation interpolation) noexcept {
    interpolation_ = interpolation;
}

std::size_t CameraPath::size() const noexcept {
    return keyframes_.size();
}

double CameraPath::durationSeconds() const noexcept {
    return keyframes_.empty() ? 0.0 : keyframes_.back().timeSeconds;
}

const std::vector<Keyframe>& CameraPath::keyframes() const noexcept {
    return keyframes_;
}

std::optional<CameraState> CameraPath::evaluate(double timeSeconds) const {
    if (!finite(timeSeconds) || keyframes_.empty()) {
        return std::nullopt;
    }
    if (timeSeconds <= keyframes_.front().timeSeconds) {
        return keyframes_.front().camera;
    }
    if (timeSeconds >= keyframes_.back().timeSeconds) {
        return keyframes_.back().camera;
    }

    const auto right = std::upper_bound(
        keyframes_.begin(), keyframes_.end(), timeSeconds,
        [](double time, const Keyframe& frame) { return time < frame.timeSeconds; });
    const auto left = right - 1;
    const auto leftIndex = static_cast<std::size_t>(left - keyframes_.begin());
    const double t = (timeSeconds - left->timeSeconds) / (right->timeSeconds - left->timeSeconds);

    return CameraState{
        interpolation_ == PositionInterpolation::Linear
            ? lerp(left->camera.position, right->camera.position, t)
            : cubicPosition(keyframes_, leftIndex, t),
        slerp(left->camera.rotation, right->camera.rotation, t),
        left->camera.fovDegrees * (1.0 - t) + right->camera.fovDegrees * t
    };
}

} // namespace ncrec
