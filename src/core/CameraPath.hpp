#pragma once

#include <cstddef>
#include <optional>
#include <vector>

namespace ncrec {

struct Vec3 {
    double x{};
    double y{};
    double z{};
};

struct Quaternion {
    double x{};
    double y{};
    double z{};
    double w{1.0};
};

struct CameraState {
    Vec3 position{};
    Quaternion rotation{};
    double fovDegrees{60.0};
};

struct Keyframe {
    double timeSeconds{};
    CameraState camera{};
};

enum class PositionInterpolation {
    Linear,
    CubicHermite
};

class CameraPath {
public:
    // Invalid states and duplicate timestamps are rejected.
    bool addKeyframe(const Keyframe& keyframe);
    void clear() noexcept;
    [[nodiscard]] bool removeKeyframe(std::size_t index) noexcept;
    void setPositionInterpolation(PositionInterpolation interpolation) noexcept;
    [[nodiscard]] PositionInterpolation positionInterpolation() const noexcept { return interpolation_; }

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] double durationSeconds() const noexcept;
    [[nodiscard]] const std::vector<Keyframe>& keyframes() const noexcept;
    [[nodiscard]] std::optional<CameraState> evaluate(double timeSeconds) const;

private:
    std::vector<Keyframe> keyframes_;
    PositionInterpolation interpolation_{PositionInterpolation::CubicHermite};
};

} // namespace ncrec
