#include "InspectPanelView.hpp"

#include "ui/BasicUi.hpp"

#include <algorithm>

namespace {
constexpr float padding = 10.f;
constexpr float headerFontSize = 17.f;
constexpr float subheaderFontSize = 12.f;
constexpr float entryTitleFontSize = 13.f;
constexpr float entryDescriptionFontSize = 12.f;

Color panelFill() {
    return Color{24, 26, 34, 238};
}

Color panelBorder() {
    return Color{220, 190, 105, 255};
}

void drawWrapped(
    const UiFont& font,
    const std::string& text,
    const Rectangle bounds,
    const float fontSize,
    const Color color,
    float& y
) {
    const std::vector<std::string> lines = BasicUi::wrapText(
        font,
        text,
        fontSize,
        bounds.width
    );

    for (const std::string& line : lines) {
        if (y + fontSize > bounds.y + bounds.height) {
            return;
        }

        BasicUi::drawText(font, line, Vector2{bounds.x, y}, fontSize, color);
        y += fontSize + 2.f;
    }
}
}

void InspectPanelView::render(
    const UiFont& font,
    const InspectPanelModel& model,
    const Rectangle bounds
) const {
    if (model.empty()) {
        return;
    }

    DrawRectangleRounded(bounds, 0.055f, 10, panelFill());
    DrawRectangleRoundedLinesEx(bounds, 0.055f, 10, 1.5f, panelBorder());

    const Rectangle content{
        bounds.x + padding,
        bounds.y + padding,
        std::max(10.f, bounds.width - padding * 2.f),
        std::max(10.f, bounds.height - padding * 2.f)
    };

    float y = content.y;

    if (!model.header.empty()) {
        drawWrapped(
            font,
            model.header,
            Rectangle{content.x, y, content.width, content.y + content.height - y},
            headerFontSize,
            Color{255, 235, 170, 255},
            y
        );
        y += 4.f;
    }

    if (!model.subheader.empty()) {
        drawWrapped(
            font,
            model.subheader,
            Rectangle{content.x, y, content.width, content.y + content.height - y},
            subheaderFontSize,
            Color{220, 220, 230, 255},
            y
        );
        y += 5.f;
    }

    for (const InspectEntry& entry : model.entries) {
        if (y >= content.y + content.height - entryDescriptionFontSize) {
            break;
        }

        if (!entry.title.empty()) {
            drawWrapped(
                font,
                entry.title,
                Rectangle{content.x, y, content.width, content.y + content.height - y},
                entryTitleFontSize,
                Color{245, 220, 140, 255},
                y
            );
        }

        if (!entry.description.empty()) {
            drawWrapped(
                font,
                entry.description,
                Rectangle{content.x, y, content.width, content.y + content.height - y},
                entryDescriptionFontSize,
                Color{205, 208, 220, 255},
                y
            );
        }

        y += 5.f;
    }
}
