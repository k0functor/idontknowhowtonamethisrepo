#pragma once

#include <raylib.h>

#include <filesystem>
#include <string>
#include <vector>

class UiFont {
public:
    UiFont() = default;
    ~UiFont();

    UiFont(const UiFont&) = delete;
    UiFont& operator=(const UiFont&) = delete;

    bool loadFromAssetsDirectory(const std::filesystem::path& assetsDirectory);

    bool available() const;
    const Font& font() const;

    const std::filesystem::path& loadedPath() const;

private:
    static std::vector<std::filesystem::path> candidatePaths(
        const std::filesystem::path& assetsDirectory
    );

    static std::vector<int> buildCodepoints();

private:
    Font font_{};
    bool available_ = false;
    std::filesystem::path loadedPath_;
};
