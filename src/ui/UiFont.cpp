#include "UiFont.hpp"

#include <stdexcept>

UiFont::~UiFont() {
    if (available_) {
        UnloadFont(font_);
        available_ = false;
    }
}

bool UiFont::loadFromAssetsDirectory(const std::filesystem::path& assetsDirectory) {
    if (available_) {
        UnloadFont(font_);
        available_ = false;
        loadedPath_.clear();
    }

    const std::vector<int> codepoints = buildCodepoints();

    for (const std::filesystem::path& path : candidatePaths(assetsDirectory)) {
        if (!std::filesystem::exists(path)) {
            continue;
        }

        font_ = LoadFontEx(
            path.string().c_str(),
            32,
            const_cast<int*>(codepoints.data()),
            static_cast<int>(codepoints.size())
        );

        if (font_.texture.id != 0) {
            SetTextureFilter(font_.texture, TEXTURE_FILTER_BILINEAR);
            available_ = true;
            loadedPath_ = path;
            return true;
        }
    }

    available_ = false;
    loadedPath_.clear();
    return false;
}

bool UiFont::available() const {
    return available_;
}

const Font& UiFont::font() const {
    if (!available_) {
        throw std::runtime_error("UI font was requested, but no font is loaded"); // NOL10N: developer UI resource diagnostic
    }

    return font_;
}

const std::filesystem::path& UiFont::loadedPath() const {
    return loadedPath_;
}

std::vector<std::filesystem::path> UiFont::candidatePaths(
    const std::filesystem::path& assetsDirectory
) {
    return {
        assetsDirectory / "fonts" / "main.ttf",
        assetsDirectory / "fonts" / "NotoSans-Regular.ttf",
        assetsDirectory / "fonts" / "NotoSans.ttf",
        assetsDirectory / "fonts" / "default.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/calibri.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"
    };
}

std::vector<int> UiFont::buildCodepoints() {
    std::vector<int> codepoints;

    auto addRange = [&codepoints](const int first, const int last) {
        for (int value = first; value <= last; ++value) {
            codepoints.push_back(value);
        }
    };

    addRange(0x0020, 0x007E); // Basic Latin
    addRange(0x00A0, 0x00FF); // Latin-1 punctuation used often enough to be annoying
    addRange(0x0400, 0x052F); // Cyrillic + Cyrillic Supplement

    codepoints.push_back(0x2013); // en dash
    codepoints.push_back(0x2014); // em dash
    codepoints.push_back(0x2018);
    codepoints.push_back(0x2019);
    codepoints.push_back(0x201C);
    codepoints.push_back(0x201D);
    codepoints.push_back(0x2022); // bullet point used by modal lists
    codepoints.push_back(0x2026); // ellipsis
    codepoints.push_back(0x2116); // numero sign

    return codepoints;
}
