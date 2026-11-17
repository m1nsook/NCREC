#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace ncrec::input {

// Portable numeric Windows virtual keys; no dependency on Windows headers.
// CET owns F10. F4, modifiers, Escape, Enter and gameplay controls are excluded.
struct ToggleKey {
    std::uint32_t vk;
    std::string_view label;
};
inline constexpr std::array<ToggleKey, 11> kToggleKeys{{
    {0x75, "F6"}, {0x76, "F7"}, {0x77, "F8"}, {0x78, "F9"},
    {0x7A, "F11"}, {0x7B, "F12"}, {0x2D, "Insert"},
    {0x24, "Home"}, {0x23, "End"}, {0x21, "PageUp"},
    {0x22, "PageDown"}
}};
inline constexpr std::uint32_t kDefaultToggleKey = 0x77;

[[nodiscard]] constexpr bool allowedToggleKey(std::uint32_t vk) noexcept {
    for (const auto& key : kToggleKeys)
        if (key.vk == vk) return true;
    return false;
}

[[nodiscard]] constexpr std::string_view toggleKeyName(std::uint32_t vk) noexcept {
    for (const auto& key : kToggleKeys)
        if (key.vk == vk) return key.label;
    return {};
}

} // namespace ncrec::input
