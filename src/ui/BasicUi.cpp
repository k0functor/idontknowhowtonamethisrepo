#include "BasicUi.hpp"

#include <algorithm>
#include <sstream>

namespace BasicUi {
namespace {
constexpr float textSpacing = 1.f;

Vector2 textSize(const UiFont& font, const std::string& text, const float fontSize) {
    if (font.available()) {
        return MeasureTextEx(font.font(), text.c_str(), fontSize, textSpacing);
    }

    return Vector2{static_cast<float>(MeasureText(text.c_str(), static_cast<int>(fontSize))), fontSize};
}
}

void drawText(
    const UiFont& font,
    const std::string& text,
    const Vector2 position,
    const float fontSize,
    const Color color
) {
    if (font.available()) {
        DrawTextEx(font.font(), text.c_str(), position, fontSize, textSpacing, color);
        return;
    }

    DrawText(text.c_str(), static_cast<int>(position.x), static_cast<int>(position.y), static_cast<int>(fontSize), color);
}

void drawCenteredText(
    const UiFont& font,
    const std::string& text,
    const Rectangle bounds,
    const float fontSize,
    const Color color
) {
    const Vector2 size = textSize(font, text, fontSize);
    const Vector2 position{
        bounds.x + (bounds.width - size.x) * 0.5f,
        bounds.y + (bounds.height - size.y) * 0.5f
    };

    drawText(font, text, position, fontSize, color);
}

float measureTextWidth(
    const UiFont& font,
    const std::string& text,
    const float fontSize
) {
    return textSize(font, text, fontSize).x;
}

bool contains(const Rectangle bounds, const Vector2 point) {
    return point.x >= bounds.x &&
        point.x <= bounds.x + bounds.width &&
        point.y >= bounds.y &&
        point.y <= bounds.y + bounds.height;
}

bool drawButton(
    const UiFont& font,
    const Rectangle bounds,
    const std::string& label,
    const Vector2 mousePosition,
    const bool enabled,
    const ButtonStyle style
) {
    const bool hovered = enabled && contains(bounds, mousePosition);
    const Color fill = !enabled
        ? style.disabledBackground
        : (hovered ? style.hoveredBackground : style.background);
    const Color textColor = enabled ? style.text : style.disabledText;

    DrawRectangleRounded(bounds, 0.18f, 8, fill);
    DrawRectangleRoundedLinesEx(bounds, 0.18f, 8, 2.f, style.border);
    drawCenteredText(font, label, bounds, 24.f, textColor);

    return enabled && hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

std::vector<std::string> wrapText(
    const UiFont& font,
    const std::string& text,
    const float fontSize,
    const float maxWidth
) {
    std::vector<std::string> lines;
    std::istringstream input(text);
    std::string word;
    std::string currentLine;

    while (input >> word) {
        const std::string candidate = currentLine.empty()
            ? word
            : currentLine + " " + word;

        if (measureTextWidth(font, candidate, fontSize) <= maxWidth || currentLine.empty()) {
            currentLine = candidate;
        } else {
            lines.push_back(currentLine);
            currentLine = word;
        }
    }

    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }

    if (lines.empty()) {
        lines.push_back({});
    }

    return lines;
}
}
