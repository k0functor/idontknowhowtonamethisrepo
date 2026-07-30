#include "ActiveItemComparisonView.hpp"

#include "active_items/ActiveItemId.hpp"
#include "ui/BasicUi.hpp"

#include <algorithm>
#include <vector>

namespace {
void drawColumn(
    const UiFont& font,
    const LocalizationManager& localization,
    const ActiveItemDatabase& activeItems,
    const std::string& itemId,
    const int charge,
    const Rectangle bounds,
    const TextId& emptyTextId
) {
    DrawRectangleRounded(bounds, 0.06f, 12, Color{37, 40, 53, 255});
    DrawRectangleRoundedLinesEx(bounds, 0.06f, 12, 2.f, Color{105, 116, 148, 255});

    if (itemId.empty() || !activeItems.contains(ActiveItemId(itemId))) {
        BasicUi::drawCenteredTextFitted(
            font,
            localization.get(emptyTextId),
            Rectangle{bounds.x + 18.f, bounds.y + 30.f, bounds.width - 36.f, bounds.height - 60.f},
            24.f,
            15.f,
            Color{176, 184, 207, 255}
        );
        return;
    }

    const ActiveItemDefinition& item = activeItems.get(ActiveItemId(itemId));
    BasicUi::drawCenteredTextFitted(
        font,
        localization.get(item.nameTextId),
        Rectangle{bounds.x + 18.f, bounds.y + 14.f, bounds.width - 36.f, 40.f},
        25.f,
        17.f,
        Color{248, 233, 184, 255}
    );
    BasicUi::drawCenteredTextFitted(
        font,
        localization.format(
            TextId("active_item.compare.charge"),
            {
                {"charge", std::to_string(std::clamp(charge, 0, item.maxCharge))},
                {"maximum", std::to_string(item.maxCharge)},
                {"cost", std::to_string(item.chargeCost)}
            }
        ),
        Rectangle{bounds.x + 18.f, bounds.y + 58.f, bounds.width - 36.f, 28.f},
        17.f,
        13.f,
        Color{175, 188, 219, 255}
    );

    const std::vector<std::string> lines = BasicUi::wrapText(
        font,
        localization.get(item.descriptionTextId),
        17.f,
        bounds.width - 38.f
    );
    float y = bounds.y + 100.f;
    for (const std::string& line : lines) {
        if (y > bounds.y + bounds.height - 28.f) {
            break;
        }
        BasicUi::drawText(font, line, Vector2{bounds.x + 20.f, y}, 17.f, Color{209, 215, 232, 255});
        y += 22.f;
    }
}
}

void ActiveItemComparisonView::render(
    const UiFont& font,
    const LocalizationManager& localization,
    const ActiveItemDatabase& activeItems,
    const RunState& run,
    const std::string& offeredItemId,
    const Rectangle bounds
) {
    const float gap = 24.f;
    const float columnWidth = (bounds.width - gap) * 0.5f;
    const Rectangle current{bounds.x, bounds.y + 34.f, columnWidth, bounds.height - 34.f};
    const Rectangle offered{bounds.x + columnWidth + gap, bounds.y + 34.f, columnWidth, bounds.height - 34.f};

    BasicUi::drawCenteredText(font, localization.get(TextId("active_item.compare.current")), Rectangle{current.x, bounds.y, current.width, 28.f}, 18.f, Color{178, 187, 213, 255});
    BasicUi::drawCenteredText(font, localization.get(TextId("active_item.compare.offered")), Rectangle{offered.x, bounds.y, offered.width, 28.f}, 18.f, Color{236, 209, 132, 255});

    drawColumn(font, localization, activeItems, run.activeItem.itemId, run.activeItem.charge, current, TextId("active_item.compare.empty"));
    drawColumn(font, localization, activeItems, offeredItemId, 0, offered, TextId("active_item.compare.unavailable"));
}
