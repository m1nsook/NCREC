#pragma once

#include "core/CameraPath.hpp"

#include <string>
#include <string_view>

namespace ncrec {

// Prototype command/timeline model. Does not read or modify the game camera.
class Session {
public:
    [[nodiscard]] std::string execute(std::string_view command);
    void advance(double deltaSeconds) noexcept;
    void seek(double seconds) noexcept;

    [[nodiscard]] const CameraPath& path() const noexcept;
    [[nodiscard]] double cursorSeconds() const noexcept;
    [[nodiscard]] bool playing() const noexcept;

private:
    CameraPath path_;
    double cursorSeconds_{};
    bool playing_{};
};

} // namespace ncrec
