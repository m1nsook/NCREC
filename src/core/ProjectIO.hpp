#pragma once

#include "core/CameraPath.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace ncrec {

// Versioned, portable camera-path JSON. All loaded keyframes go back through
// CameraPath validation; malformed or unsupported files fail without replacing
// the currently edited project.
struct ProjectIO {
    static bool save(const CameraPath& path, const std::filesystem::path& file,
                     std::string& error);
    [[nodiscard]] static std::optional<CameraPath> load(
        const std::filesystem::path& file, std::string& error);
};

} // namespace ncrec
