#pragma once

#include "core/CameraPath.hpp"
#include "core/CameraRig.hpp"

#include <string>
#include <string_view>
#include <filesystem>

namespace ncrec {

// Editor-side virtual camera and timeline model. The rig is deliberately
// independent of the game view until a supported adapter is available.
class Session {
public:
    [[nodiscard]] std::string execute(std::string_view command);
    void setProjectDirectory(std::filesystem::path directory);
    void advance(double deltaSeconds) noexcept;
    void seek(double seconds) noexcept;

    [[nodiscard]] bool setCameraPose(const CameraState& state) noexcept;
    [[nodiscard]] bool addKeyframe(double seconds);
    [[nodiscard]] bool removeKeyframe(std::size_t index);
    [[nodiscard]] const CameraRig& rig() const noexcept;
    [[nodiscard]] const CameraPath& path() const noexcept;
    [[nodiscard]] double cursorSeconds() const noexcept;
    [[nodiscard]] bool playing() const noexcept;

private:
    CameraPath path_;
    CameraRig rig_;
    double cursorSeconds_{};
    bool playing_{};
    std::filesystem::path projects_;
};

} // namespace ncrec
