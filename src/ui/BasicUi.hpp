#pragma once

#include "ui/UiFont.hpp"

#include <raylib.h>

#include <string>
#include <vector>

namespace BasicUi {
struct ButtonStyle {
    Color background{45, 48, 58, 255};
    Color hoveredBackground{68, 72, 86, 255};
    Color disabledBackground{35, 36, 42, 255};
    Color border{130, 136, 160, 255};
    Color text{235, 235, 242, 255};
    Color disabledText{120, 124, 140, 255};
};

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

float measureTextWidth(
    const UiFont& font,
    const std::string& text,
    float fontSize
);

bool contains(Rectangle bounds, Vector2 point);

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
