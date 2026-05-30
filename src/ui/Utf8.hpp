#pragma once

#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>

namespace UiUtf8 {
namespace detail {
inline bool isContinuationByte(const unsigned char value) {
    return (value & 0xC0u) == 0x80u;
}

inline std::size_t expectedCodepointLength(const unsigned char firstByte) {
    if ((firstByte & 0x80u) == 0u) {
        return 1u;
    }
    if ((firstByte & 0xE0u) == 0xC0u) {
        return 2u;
    }
    if ((firstByte & 0xF0u) == 0xE0u) {
        return 3u;
    }
    if ((firstByte & 0xF8u) == 0xF0u) {
        return 4u;
    }

    return 1u;
}
}

inline std::size_t nextCodepointOffset(const std::string_view text, const std::size_t offset) {
    if (offset >= text.size()) {
        return text.size();
    }

    const unsigned char firstByte = static_cast<unsigned char>(text[offset]);
    const std::size_t length = detail::expectedCodepointLength(firstByte);

    if (offset + length > text.size()) {
        return offset + 1u;
    }

    for (std::size_t i = 1u; i < length; ++i) {
        if (!detail::isContinuationByte(static_cast<unsigned char>(text[offset + i]))) {
            return offset + 1u;
        }
    }

    return offset + length;
}

inline std::size_t codepointCount(const std::string_view text) {
    std::size_t result = 0u;
    for (std::size_t offset = 0u; offset < text.size(); offset = nextCodepointOffset(text, offset)) {
        ++result;
    }

    return result;
}

inline std::string takeCodepoints(const std::string_view text, const std::size_t maxCodepoints) {
    std::size_t offset = 0u;
    std::size_t count = 0u;

    while (offset < text.size() && count < maxCodepoints) {
        offset = nextCodepointOffset(text, offset);
        ++count;
    }

    return std::string(text.substr(0u, offset));
}

inline std::string truncateWithEllipsis(const std::string_view text, const std::size_t maxCodepoints) {
    if (codepointCount(text) <= maxCodepoints) {
        return std::string(text);
    }

    if (maxCodepoints <= 3u) {
        return takeCodepoints(text, maxCodepoints);
    }

    return takeCodepoints(text, maxCodepoints - 3u) + "...";
}

inline std::string wrapByCodepoints(
    const std::string& text,
    const std::size_t lineLength,
    const std::size_t maxLines
) {
    if (lineLength == 0u || maxLines == 0u) {
        return {};
    }

    std::istringstream input(text);
    std::string word;
    std::string result;
    std::string line;
    std::size_t lines = 0u;

    while (input >> word) {
        const std::size_t lineCount = codepointCount(line);
        const std::size_t wordCount = codepointCount(word);

        if (line.empty()) {
            line = word;
        } else if (lineCount + 1u + wordCount <= lineLength) {
            line += " " + word;
        } else {
            if (!result.empty()) {
                result += '\n';
            }

            result += truncateWithEllipsis(line, lineLength);
            ++lines;

            if (lines >= maxLines) {
                return result;
            }

            line = word;
        }
    }

    if (!line.empty() && lines < maxLines) {
        if (!result.empty()) {
            result += '\n';
        }

        result += truncateWithEllipsis(line, lineLength);
    }

    return result;
}
}
