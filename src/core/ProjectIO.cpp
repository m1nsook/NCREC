#include "core/ProjectIO.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <limits>
#include <system_error>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace ncrec {
namespace {
constexpr std::size_t kMaxKeyframes = 4096;
constexpr std::uintmax_t kMaxFileBytes = 4 * 1024 * 1024;
constexpr char kFormat[] = "NCREC.CameraPath";
constexpr int kVersion = 1;

bool atomicReplace(const std::filesystem::path& temporary,
                   const std::filesystem::path& finalFile) {
#ifdef _WIN32
    return MoveFileExW(temporary.c_str(), finalFile.c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    std::error_code ec;
    std::filesystem::rename(temporary, finalFile, ec);
    return !ec;
#endif
}

} // namespace

bool ProjectIO::save(const CameraPath& path, const std::filesystem::path& file,
                     std::string& error) {
    error.clear();
    if (path.size() > kMaxKeyframes) {
        error = "Too many keyframes (maximum 4096).";
        return false;
    }
    std::filesystem::path temporary = file;
    temporary += ".tmp";
    try {
        nlohmann::json frames = nlohmann::json::array();
        for (const Keyframe& k : path.keyframes()) {
            const auto& p = k.camera.position;
            const auto& q = k.camera.rotation;
            frames.push_back({
                {"time", k.timeSeconds},
                {"position", {p.x, p.y, p.z}},
                {"rotation", {q.x, q.y, q.z, q.w}},
                {"fov", k.camera.fovDegrees}
            });
        }
        nlohmann::json document = {
            {"format", kFormat},
            {"version", kVersion},
            {"positionInterpolation", path.positionInterpolation() == PositionInterpolation::Linear
                                      ? "linear" : "cubic_hermite"},
            {"keyframes", std::move(frames)}
        };
        const std::string body = document.dump(2);
        if (body.size() > kMaxFileBytes) {
            error = "Project exceeds the 4 MiB size limit.";
            return false;
        }
        std::filesystem::create_directories(file.parent_path());
        {
            std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
            if (!stream || !stream.write(body.data(),
                    static_cast<std::streamsize>(body.size())) || !stream.flush()) {
                error = "Could not write the temporary camera project.";
                return false;
            }
        }
        if (!atomicReplace(temporary, file)) {
            error = "Could not commit the project (check folder permissions).";
            std::error_code ec;
            std::filesystem::remove(temporary, ec);
            return false;
        }
        return true;
    } catch (const std::exception& exception) {
        error = std::string("Save failed: ") + exception.what();
        return false;
    }
}

std::optional<CameraPath> ProjectIO::load(const std::filesystem::path& file,
                                          std::string& error) {
    error.clear();
    try {
        if (!std::filesystem::exists(file)) {
            error = "Project not found.";
            return std::nullopt;
        }
        if (std::filesystem::file_size(file) > kMaxFileBytes) {
            error = "Project file exceeds the 4 MiB limit.";
            return std::nullopt;
        }
        std::ifstream stream(file, std::ios::binary);
        if (!stream) {
            error = "Project cannot be opened.";
            return std::nullopt;
        }
        const auto json = nlohmann::json::parse(stream);
        if (!json.is_object() || !json.contains("format")
            || json.at("format") != kFormat || !json.contains("version")
            || json.at("version") != kVersion) {
            error = "Unsupported NCREC project format/version.";
            return std::nullopt;
        }
        const auto& frames = json.at("keyframes");
        if (!frames.is_array() || frames.size() > kMaxKeyframes) {
            error = "Invalid number of keyframes.";
            return std::nullopt;
        }
        CameraPath path;
        const std::string mode = json.at("positionInterpolation").get<std::string>();
        if (mode == "linear")
            path.setPositionInterpolation(PositionInterpolation::Linear);
        else if (mode == "cubic_hermite")
            path.setPositionInterpolation(PositionInterpolation::CubicHermite);
        else {
            error = "Unsupported position interpolation.";
            return std::nullopt;
        }

        for (const auto& frame : frames) {
            const auto& p = frame.at("position");
            const auto& q = frame.at("rotation");
            if (!p.is_array() || p.size() != 3 || !q.is_array() || q.size() != 4) {
                error = "Invalid camera position or quaternion array.";
                return std::nullopt;
            }
            Keyframe keyframe{};
            keyframe.timeSeconds = frame.at("time").get<double>();
            keyframe.camera.position = {
                p.at(0).get<double>(), p.at(1).get<double>(), p.at(2).get<double>()
            };
            keyframe.camera.rotation = {
                q.at(0).get<double>(), q.at(1).get<double>(),
                q.at(2).get<double>(), q.at(3).get<double>()
            };
            keyframe.camera.fovDegrees = frame.at("fov").get<double>();
            if (!path.addKeyframe(keyframe)) {
                error = "Invalid or duplicate camera keyframe.";
                return std::nullopt;
            }
        }
        return path;
    } catch (const std::exception& exception) {
        error = std::string("Invalid project: ") + exception.what();
        return std::nullopt;
    }
}

} // namespace ncrec
