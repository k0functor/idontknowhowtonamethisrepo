#pragma once

#include "ui/UiFont.hpp"
#include "ui/UiTheme.hpp"

#include <raylib.h>

#include <string>
#include <vector>

namespace BasicUi {
struct ButtonStyle {
    Color background{UiTheme::toneFill(UiTheme::Tone::Neutral)};
    Color hoveredBackground{UiTheme::toneHoverFill(UiTheme::Tone::Neutral)};
    Color disabledBackground{UiTheme::toneFill(UiTheme::Tone::Disabled)};
    Color border{UiTheme::toneBorder(UiTheme::Tone::Neutral)};
    Color text{UiTheme::toneText(UiTheme::Tone::Neutral)};
    Color disabledText{UiTheme::toneText(UiTheme::Tone::Disabled)};
};

ButtonStyle buttonStyle(UiTheme::Tone tone);

void drawText(
    const UiFont& font,
    const std::string& text,
    Vector2 position,
    float fontSize,
    Color color
);

void drawCenteredText(
    const UiFont& font,
    const std::string& text,
    Rectangle bounds,
    float fontSize,
    Color color
);

void drawTextFitted(
    const UiFont& font,
    const std::string& text,
    Vector2 position,
    float maxWidth,
    float preferredFontSize,
    float minimumFontSize,
    Color color
);

void drawCenteredTextFitted(
    const UiFont& font,
    const std::string& text,
    Rectangle bounds,
    float preferredFontSize,
    float minimumFontSize,
    Color color
);

float measureTextWidth(
    const UiFont& font,
    const std::string& text,
    float fontSize
);

bool contains(Rectangle bounds, Vector2 point);

void drawPanel(
    Rectangle bounds,
    bool raised = false,
    UiTheme::Tone tone = UiTheme::Tone::Neutral,
    float roundness = UiTheme::panelRoundness
);

void drawChip(
    const UiFont& font,
    Rectangle bounds,
    const std::string& label,
    UiTheme::Tone tone = UiTheme::Tone::Neutral,
    float fontSize = 13.f
);

void drawProgressBar(
    Rectangle bounds,
    float ratio,
    UiTheme::Tone tone,
    bool showMarkers = false
);

bool drawButton(
    const UiFont& font,
    Rectangle bounds,
    const std::string& label,
    Vector2 mousePosition,
    bool enabled = true,
    ButtonStyle style = {}
);

std::vector<std::string> wrapText(
    const UiFont& font,
    const std::string& text,
    float fontSize,
    float maxWidth
);
}
