#include "core/Session.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace ncrec {
namespace {

[[nodiscard]] std::string usage() {
    return "Commands: help, status, path demo, path list, path clear, "
           "path play, path pause, path seek <seconds>, path sample <seconds>";
}

[[nodiscard]] bool parseTime(std::istringstream& input, double& output) {
    std::string trailing;
    return (input >> output) && std::isfinite(output) && output >= 0.0
        && !(input >> trailing);
}

} // namespace

std::string Session::execute(std::string_view command) {
    std::istringstream input{std::string(command)};
    std::string primary;
    input >> primary;
    if (primary == "help") {
        return usage();
    }
    if (primary == "status") {
        std::ostringstream output;
        output << "NCREC prototype | camera adapter: NOT CONNECTED | "
               << path_.size() << " keyframes | cursor " << std::fixed
               << std::setprecision(3) << cursorSeconds_ << "s | "
               << (playing_ ? "playing" : "paused");
        return output.str();
    }
    if (primary != "path") {
        return primary.empty() ? usage() : "Unknown command. Type help.";
    }

    std::string action;
    input >> action;
    if (action == "demo") {
        path_.clear();
        path_.addKeyframe({0.0, {{0, 0, 0}, {}, 65}});
        path_.addKeyframe({2.5, {{3, 2, 1}, {}, 55}});
        path_.addKeyframe({5.0, {{8, 0, 3}, {}, 40}});
        cursorSeconds_ = 0.0;
        playing_ = false;
        return "Demo path loaded (3 points / 5 seconds). Not applied to the game camera.";
    }
    if (action == "clear") {
        path_.clear();
        cursorSeconds_ = 0.0;
        playing_ = false;
        return "Path cleared.";
    }
    if (action == "list") {
        std::ostringstream output;
        output << path_.size() << " keyframe(s):";
        for (const auto& frame : path_.keyframes()) {
            output << " " << std::fixed << std::setprecision(3) << frame.timeSeconds << "s";
        }
        return output.str();
    }
    if (action == "play") {
        if (path_.size() < 2 || path_.durationSeconds() <= 0.0) {
            return "Playback needs at least two keyframes with a positive duration.";
        }
        if (cursorSeconds_ >= path_.durationSeconds()) {
            cursorSeconds_ = 0.0;
        }
        playing_ = true;
        return "Playing timeline preview (not controlling the game camera).";
    }
    if (action == "pause") {
        playing_ = false;
        return "Playback paused.";
    }
    if (action == "seek" || action == "sample") {
        double seconds = 0.0;
        if (!parseTime(input, seconds)) {
            return "Expected a finite, nonnegative time in seconds.";
        }
        if (action == "seek") {
            seek(seconds);
            return "Timeline cursor updated.";
        }
        const auto camera = path_.evaluate(seconds);
        if (!camera) {
            return "No camera keyframes to sample.";
        }
        std::ostringstream output;
        output << std::fixed << std::setprecision(3)
               << "pos=(" << camera->position.x << ", " << camera->position.y
               << ", " << camera->position.z << ") fov=" << camera->fovDegrees;
        return output.str();
    }

    return usage();
}

void Session::advance(double deltaSeconds) noexcept {
    if (!playing_ || !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0) {
        return;
    }
    cursorSeconds_ = std::min(path_.durationSeconds(), cursorSeconds_ + deltaSeconds);
    if (cursorSeconds_ >= path_.durationSeconds()) {
        playing_ = false;
    }
}

void Session::seek(double seconds) noexcept {
    if (std::isfinite(seconds)) {
        cursorSeconds_ = std::clamp(seconds, 0.0, path_.durationSeconds());
    }
}

const CameraPath& Session::path() const noexcept {
    return path_;
}

double Session::cursorSeconds() const noexcept {
    return cursorSeconds_;
}

bool Session::playing() const noexcept {
    return playing_;
}

} // namespace ncrec
