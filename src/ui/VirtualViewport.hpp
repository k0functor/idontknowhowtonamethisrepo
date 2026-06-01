#pragma once

namespace VirtualViewport {
    inline constexpr int widthPixels = 1920;
    inline constexpr int heightPixels = 1080;

    inline int width() noexcept {
        return widthPixels;
    }

    inline int height() noexcept {
        return heightPixels;
    }

    inline float widthF() noexcept {
        return static_cast<float>(widthPixels);
    }

    inline float heightF() noexcept {
        return static_cast<float>(heightPixels);
    }
}
