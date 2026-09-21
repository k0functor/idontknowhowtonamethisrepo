#include "BasicUi.hpp"

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>

namespace BasicUi {
namespace {
constexpr float textSpacing = 1.f;

Vector2 textSize(const UiFont& font, const std::string& text, const float fontSize) {
    if (font.available()) {
        return MeasureTextEx(font.font(), text.c_str(), fontSize, textSpacing);
    }

    return Vector2{static_cast<float>(MeasureText(text.c_str(), static_cast<int>(fontSize))), fontSize};
}

void popUtf8Codepoint(std::string& text) {
    if (text.empty()) {
        return;
    }

    std::size_t start = text.size() - 1u;
    while (start > 0u && (static_cast<unsigned char>(text[start]) & 0xC0u) == 0x80u) {
        --start;
    }
    text.erase(start);
}

std::string ellipsizeToWidth(
    const UiFont& font,
    const std::string& text,
    const float fontSize,
    const float maxWidth
) {
    if (measureTextWidth(font, text, fontSize) <= maxWidth) {
        return text;
    }

    constexpr const char* suffix = "...";
    std::string result = text;
    while (!result.empty() && measureTextWidth(font, result + suffix, fontSize) > maxWidth) {
        popUtf8Codepoint(result);
    }

    return result.empty() ? std::string(suffix) : result + suffix;
}

float fittedFontSize(
    const UiFont& font,
    const std::string& text,
    const float preferredFontSize,
    const float minimumFontSize,
    const float maxWidth
) {
    float size = preferredFontSize;
    while (size > minimumFontSize && measureTextWidth(font, text, size) > maxWidth) {
        size -= 1.f;
    }
    return std::max(minimumFontSize, size);
}
}

ButtonStyle buttonStyle(const UiTheme::Tone tone) {
    return ButtonStyle{
        UiTheme::toneFill(tone),
        UiTheme::toneHoverFill(tone),
        UiTheme::toneFill(UiTheme::Tone::Disabled),
        UiTheme::toneBorder(tone),
        UiTheme::toneText(tone),
        UiTheme::toneText(UiTheme::Tone::Disabled)
    };
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

void drawTextFitted(
    const UiFont& font,
    const std::string& text,
    const Vector2 position,
    const float maxWidth,
    const float preferredFontSize,
    const float minimumFontSize,
    const Color color
) {
    const float fontSize = fittedFontSize(font, text, preferredFontSize, minimumFontSize, maxWidth);
    drawText(font, ellipsizeToWidth(font, text, fontSize, maxWidth), position, fontSize, color);
}

void drawCenteredTextFitted(
    const UiFont& font,
    const std::string& text,
    const Rectangle bounds,
    const float preferredFontSize,
    const float minimumFontSize,
    const Color color
) {
    const float fontSize = fittedFontSize(font, text, preferredFontSize, minimumFontSize, bounds.width);
    const std::string fittedText = ellipsizeToWidth(font, text, fontSize, bounds.width);
    const Vector2 size = textSize(font, fittedText, fontSize);
    const Vector2 position{
        bounds.x + (bounds.width - size.x) * 0.5f,
        bounds.y + (bounds.height - size.y) * 0.5f
    };

    drawText(font, fittedText, position, fontSize, color);
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

void drawPanel(
    const Rectangle bounds,
    const bool raised,
    const UiTheme::Tone tone,
    const float roundness
) {
    const Color fill = tone == UiTheme::Tone::Neutral
        ? (raised ? UiTheme::panelRaised : UiTheme::panel)
        : UiTheme::toneFill(tone);
    const Color panelBorder = tone == UiTheme::Tone::Neutral
        ? UiTheme::border
        : UiTheme::toneBorder(tone);

    DrawRectangleRounded(bounds, roundness, 10, fill);
    DrawRectangleRoundedLinesEx(bounds, roundness, 10, UiTheme::borderThickness, panelBorder);
}

void drawChip(
    const UiFont& font,
    const Rectangle bounds,
    const std::string& label,
    const UiTheme::Tone tone,
    const float fontSize
) {
    DrawRectangleRounded(bounds, UiTheme::chipRoundness, 8, UiTheme::toneFill(tone));
    DrawRectangleRoundedLinesEx(bounds, UiTheme::chipRoundness, 8, 1.5f, UiTheme::toneBorder(tone));
    drawCenteredTextFitted(
        font,
        label,
        Rectangle{bounds.x + 7.f, bounds.y, std::max(1.f, bounds.width - 14.f), bounds.height},
        fontSize,
        std::max(10.f, fontSize - 3.f),
        UiTheme::toneText(tone)
    );
}

void drawProgressBar(
    const Rectangle bounds,
    const float ratio,
    const UiTheme::Tone tone,
    const bool showMarkers
) {
    const float clamped = std::clamp(ratio, 0.f, 1.f);
    DrawRectangleRounded(bounds, 0.45f, 8, UiTheme::panelInset);
    if (clamped > 0.f) {
        DrawRectangleRounded(
            Rectangle{bounds.x, bounds.y, bounds.width * clamped, bounds.height},
            0.45f,
            8,
            UiTheme::toneBorder(tone)
        );
    }
    DrawRectangleRoundedLinesEx(bounds, 0.45f, 8, 1.f, UiTheme::borderSoft);

    if (showMarkers) {
        for (const float marker : {0.2f, 0.4f, 0.6f, 0.8f}) {
            const float x = bounds.x + bounds.width * marker;
            DrawLineEx(
                Vector2{x, bounds.y + 1.f},
                Vector2{x, bounds.y + bounds.height - 1.f},
                1.f,
                UiTheme::withAlpha(UiTheme::textPrimary, 0.55f)
            );
        }
    }
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

    DrawRectangleRounded(bounds, UiTheme::controlRoundness, 8, fill);
    DrawRectangleRoundedLinesEx(bounds, UiTheme::controlRoundness, 8, UiTheme::borderThickness, style.border);
    drawCenteredTextFitted(font, label, Rectangle{bounds.x + 10.f, bounds.y, bounds.width - 20.f, bounds.height}, 27.f, 18.f, textColor);

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
