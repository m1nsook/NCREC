#include "core/Session.hpp"
#include "core/ProjectIO.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

namespace ncrec {
namespace {

[[nodiscard]] std::string usage() {
    return "Commands: help, status, camera pose, camera pos <x> <y> <z>, "
           "camera fov <degrees>, camera rotate <yaw> <pitch> <roll>, "
           "path demo, path add <seconds>, path remove <index>, path list, "
           "path clear, path play, path pause, path seek <seconds>, path sample <seconds>, "
           "path save <name>, path load <name>";
}

template<typename... Args>
[[nodiscard]] bool parseNumbers(std::istringstream& input, Args&... args) {
    const bool parsed = (static_cast<bool>(input >> args) && ...);
    if (!parsed) return false;
    std::string extra;
    if (input >> extra) return false;
    return (std::isfinite(static_cast<double>(args)) && ...);
}

[[nodiscard]] std::string cameraDescription(const CameraState& state) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(3)
        << "pos=(" << state.position.x << ", " << state.position.y
        << ", " << state.position.z << "), "
        << "quat=(" << state.rotation.x << ", " << state.rotation.y
        << ", " << state.rotation.z << ", " << state.rotation.w << "), "
        << "fov=" << state.fovDegrees;
    return out.str();
}

} // namespace

std::string Session::execute(std::string_view command) {
    std::istringstream input{std::string(command)};
    std::string primary;
    input >> primary;
    if (primary == "help") return usage();
    if (primary == "status") {
        std::ostringstream output;
        output << "NCREC preview | game camera: NOT CONNECTED | "
               << path_.size() << " keyframes | cursor " << std::fixed
               << std::setprecision(3) << cursorSeconds_ << "s | "
               << (playing_ ? "playing" : "paused");
        return output.str();
    }
    if (primary == "camera") {
        std::string action;
        input >> action;
        if (action == "pose") return cameraDescription(rig_.pose());
        if (action == "pos") {
            double x{}, y{}, z{};
            if (!parseNumbers(input, x, y, z)) return "Usage: camera pos <x> <y> <z>";
            CameraState state = rig_.pose();
            state.position = {x, y, z};
            return setCameraPose(state) ? "Virtual camera position updated." : "Invalid position.";
        }
        if (action == "fov") {
            double fov{};
            if (!parseNumbers(input, fov)) return "Usage: camera fov <degrees>";
            CameraState state = rig_.pose();
            state.fovDegrees = fov;
            return setCameraPose(state) ? "Virtual camera FOV updated." : "FOV must be between 0 and 180.";
        }
        if (action == "rotate") {
            double yaw{}, pitch{}, roll{};
            if (!parseNumbers(input, yaw, pitch, roll)) {
                return "Usage: camera rotate <yaw> <pitch> <roll>";
            }
            CameraRig changed = rig_;
            if (!changed.step({0,0,0,yaw,pitch,roll}, 1.0, 0)) return "Invalid rotation.";
            rig_ = changed;
            return "Virtual camera orientation updated.";
        }
        return usage();
    }
    if (primary != "path") return primary.empty() ? usage() : "Unknown command. Type help.";

    std::string action;
    input >> action;
    if (action == "demo") {
        path_.clear();
        path_.addKeyframe({0.0, {{0, 0, 0}, {}, 65}});
        path_.addKeyframe({2.5, {{3, 2, 1}, {}, 55}});
        path_.addKeyframe({5.0, {{8, 0, 3}, {}, 40}});
        cursorSeconds_ = 0.0;
        playing_ = false;
        if (const auto pose = path_.evaluate(0.0)) { (void)rig_.setPose(*pose); }
        return "Demo path loaded (3 points / 5 seconds). Game camera is unaffected.";
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
        for (const auto& frame : path_.keyframes())
            output << " " << std::fixed << std::setprecision(3) << frame.timeSeconds << "s";
        return output.str();
    }

    if (action == "save" || action == "load") {
        std::string name, trailing;
        if (!(input >> name) || (input >> trailing) || name.size() > 64
            || !std::all_of(name.begin(), name.end(), [](char ch) {
                return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
                    || (ch >= '0' && ch <= '9') || ch == '-' || ch == '_';
            })) {
            return "Project names must be 1-64 letters, numbers, '_' or '-'.";
        }
        if (projects_.empty()) return "Project storage is not configured.";
        const auto file = projects_ / (name + ".ncrec.json");
        std::string error;
        if (action == "save") {
            return ProjectIO::save(path_, file, error)
                ? "Saved camera path: " + name : "Save failed: " + error;
        }
        auto loaded = ProjectIO::load(file, error);
        if (!loaded) return "Load failed: " + error;
        path_ = std::move(*loaded);
        playing_ = false;
        cursorSeconds_ = 0.0;
        seek(0.0);
        return "Loaded camera path: " + name;
    }

    if (action == "add") {
        double seconds{};
        if (!parseNumbers(input, seconds) || seconds < 0)
            return "Usage: path add <nonnegative seconds>";
        return addKeyframe(seconds) ? "Virtual camera pose captured as keyframe."
                                    : "Invalid or duplicate keyframe timestamp.";
    }
    if (action == "remove") {
        long long index{};
        if (!parseNumbers(input, index) || index < 0)
            return "Usage: path remove <zero-based index>";
        return removeKeyframe(static_cast<std::size_t>(index))
            ? "Keyframe removed." : "Keyframe index not found.";
    }
    if (action == "play") {
        if (path_.size() < 2 || path_.durationSeconds() <= 0.0)
            return "Playback needs at least two timed keyframes.";
        if (cursorSeconds_ >= path_.durationSeconds()) cursorSeconds_ = 0.0;
        playing_ = true;
        seek(cursorSeconds_);
        return "Playing virtual camera path (game camera unaffected).";
    }
    if (action == "pause") {
        playing_ = false;
        return "Playback paused.";
    }
    if (action == "seek" || action == "sample") {
        double seconds{};
        if (!parseNumbers(input, seconds) || seconds < 0)
            return "Expected a finite, nonnegative time in seconds.";
        if (action == "seek") {
            seek(seconds);
            return "Timeline cursor updated.";
        }
        const auto pose = path_.evaluate(seconds);
        return pose ? cameraDescription(*pose) : "No camera keyframes to sample.";
    }
    return usage();
}

void Session::advance(double deltaSeconds) noexcept {
    if (!playing_ || !std::isfinite(deltaSeconds) || deltaSeconds <= 0) return;
    seek(std::min(path_.durationSeconds(), cursorSeconds_ + deltaSeconds));
    if (cursorSeconds_ >= path_.durationSeconds()) playing_ = false;
}

void Session::seek(double seconds) noexcept {
    if (!std::isfinite(seconds)) return;
    cursorSeconds_ = std::clamp(seconds, 0.0, path_.durationSeconds());
    if (const auto pose = path_.evaluate(cursorSeconds_)) (void)rig_.setPose(*pose);
}

void Session::setProjectDirectory(std::filesystem::path directory) {
    projects_ = std::move(directory);
}

bool Session::setCameraPose(const CameraState& state) noexcept { return rig_.setPose(state); }

bool Session::addKeyframe(double seconds) {
    if (!std::isfinite(seconds) || seconds < 0.0) return false;
    return path_.addKeyframe({seconds, rig_.pose()});
}

bool Session::removeKeyframe(std::size_t index) {
    if (!path_.removeKeyframe(index)) return false;
    seek(cursorSeconds_);
    if (path_.size() < 2) playing_ = false;
    return true;
}

const CameraRig& Session::rig() const noexcept { return rig_; }
const CameraPath& Session::path() const noexcept { return path_; }
double Session::cursorSeconds() const noexcept { return cursorSeconds_; }
bool Session::playing() const noexcept { return playing_; }

} // namespace ncrec
