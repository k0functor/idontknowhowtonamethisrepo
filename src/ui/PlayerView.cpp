#include "PlayerView.hpp"

#include "ui/UiTheme.hpp"
#include "ui/Utf8.hpp"

#include <algorithm>
#include <sstream>
#include <string>

namespace {
std::string statusChipText(const StatusViewModel& status) {
    std::string text = UiUtf8::truncateWithEllipsis(status.name, 8);
    if (status.amount > 0) {
        text += " " + std::to_string(status.amount);
    }
    return text;
}

constexpr std::size_t maxVisibleRelicSlots = 6;
constexpr float relicChipHeight = 22.f;
constexpr float relicChipGap = 6.f;

std::string relicChipText(
    const RelicViewModel& relic,
    const std::size_t index,
    const std::size_t totalCount
) {
    if (totalCount > maxVisibleRelicSlots && index + 1 == maxVisibleRelicSlots) {
        return "+" + std::to_string(totalCount - maxVisibleRelicSlots + 1);
    }

    return UiUtf8::truncateWithEllipsis(relic.name, 12);
}

bool relicSlotIsOverflow(
    const std::size_t index,
    const std::size_t totalCount
) {
    return totalCount > maxVisibleRelicSlots && index + 1 == maxVisibleRelicSlots;
}
}

void PlayerView::setModel(PlayerViewModel model) {
    model_ = std::move(model);
}

const PlayerViewModel& PlayerView::model() const {
    return model_;
}

void PlayerView::setPosition(const Vector2 position) {
    position_ = position;
}

void PlayerView::setSize(const Vector2 size) {
    size_ = size;
}

void PlayerView::setStatusesOnRight(const bool statusesOnRight) {
    statusesOnRight_ = statusesOnRight;
}

bool PlayerView::contains(const Vector2 worldPosition) const {
    return CheckCollisionPointRec(worldPosition, bounds());
}

void PlayerView::render(const Font* font, const bool hovered) const {
    const Vector2 renderPosition{position_.x + model_.renderOffset.x, position_.y + model_.renderOffset.y};
    const Rectangle body{renderPosition.x, renderPosition.y, size_.x, size_.y};
    const Color fill = model_.alive ? Color{48, 72, 82, 255} : UiTheme::toneFill(UiTheme::Tone::Disabled);
    const Color outline = model_.previewTarget
        ? UiTheme::toneBorder(UiTheme::Tone::Accent)
        : (model_.targetable
            ? UiTheme::toneBorder(UiTheme::Tone::Positive)
            : (model_.activeTurn
                ? UiTheme::toneBorder(UiTheme::Tone::Accent)
                : (hovered ? UiTheme::toneText(UiTheme::Tone::Accent) : UiTheme::textSecondary)));
    const float outlineThickness = model_.previewTarget ? 5.f : (model_.targetable ? 3.5f : (model_.activeTurn ? 4.5f : (hovered ? 4.f : 2.f)));

    DrawRectangleRounded(body, 0.12f, 12, fill);
    DrawRectangleRoundedLinesEx(body, 0.12f, 12, outlineThickness, outline);

    const float hpRatio = model_.maxHp <= 0
        ? 0.f
        : static_cast<float>(std::max(0, model_.currentHp)) / static_cast<float>(model_.maxHp);

    const Rectangle hpBack{renderPosition.x + 14.f, renderPosition.y + size_.y - 36.f, size_.x - 28.f, 18.f};
    DrawRectangleRec(hpBack, UiTheme::panelInset);

    const Rectangle hpFill{hpBack.x, hpBack.y, hpBack.width * hpRatio, hpBack.height};
    DrawRectangleRec(hpFill, UiTheme::toneBorder(UiTheme::Tone::Positive));

    if (font == nullptr) {
        return;
    }


    if (model_.maxEnergy > 0) {
        const float energyRadius = 26.f;
        const int energyCenterX = static_cast<int>(renderPosition.x + size_.x * 0.5f);
        const int energyCenterY = static_cast<int>(renderPosition.y + size_.y + energyRadius + 8.f);
        DrawCircle(energyCenterX, energyCenterY, energyRadius, UiTheme::toneFill(UiTheme::Tone::Primary));
        DrawCircleLines(energyCenterX, energyCenterY, energyRadius, UiTheme::toneBorder(UiTheme::Tone::Primary));

        const std::string energyText = std::to_string(model_.currentEnergy) + "/" + std::to_string(model_.maxEnergy);
        const Vector2 textSize = MeasureTextEx(*font, energyText.c_str(), 17.f, 1.f);
        DrawTextEx(
            *font,
            energyText.c_str(),
            Vector2{static_cast<float>(energyCenterX) - textSize.x * 0.5f, static_cast<float>(energyCenterY) - textSize.y * 0.5f},
            17.f,
            1.f,
            UiTheme::toneText(UiTheme::Tone::Primary)
        );
    }

    if (!model_.relics.empty()) {
        const Rectangle firstRelicBounds = relicBounds(0).value_or(Rectangle{renderPosition.x + 14.f, renderPosition.y + size_.y + 64.f, size_.x - 28.f, relicChipHeight});
        DrawTextEx(
            *font,
            model_.relicsLabel.c_str(),
            Vector2{firstRelicBounds.x, firstRelicBounds.y - 18.f},
            12.f,
            1.f,
            UiTheme::toneText(UiTheme::Tone::Accent)
        );

        const std::size_t visibleCount = visibleRelicSlotCount();
        for (std::size_t i = 0; i < visibleCount; ++i) {
            const std::optional<Rectangle> chipBounds = relicBounds(i);
            if (!chipBounds.has_value()) {
                continue;
            }

            const bool overflow = relicSlotIsOverflow(i, model_.relics.size());
            const Color fill = overflow ? UiTheme::toneFill(UiTheme::Tone::Disabled) : UiTheme::toneFill(UiTheme::Tone::Accent);
            const Color border = overflow ? UiTheme::toneBorder(UiTheme::Tone::Disabled) : UiTheme::toneBorder(UiTheme::Tone::Accent);
            DrawRectangleRounded(*chipBounds, 0.28f, 8, fill);
            DrawRectangleRoundedLinesEx(*chipBounds, 0.28f, 8, 1.4f, border);

            const RelicViewModel& relic = model_.relics[std::min(i, model_.relics.size() - 1)];
            const std::string text = relicChipText(relic, i, model_.relics.size());
            DrawTextEx(
                *font,
                text.c_str(),
                Vector2{chipBounds->x + 7.f, chipBounds->y + 4.f},
                12.f,
                1.f,
                UiTheme::toneText(UiTheme::Tone::Accent)
            );
        }
    }

    DrawTextEx(*font, model_.name.c_str(), Vector2{renderPosition.x + 14.f, renderPosition.y + 12.f}, 20.f, 1.f, WHITE);

    if (model_.activeTurn) {
        DrawTextEx(*font, model_.activeTurnLabel.c_str(), Vector2{renderPosition.x + size_.x - 78.f, renderPosition.y + 14.f}, 14.f, 1.f, UiTheme::toneText(UiTheme::Tone::Accent));
    }

    const std::string hpText = std::to_string(model_.currentHp) + "/" + std::to_string(model_.maxHp);
    DrawTextEx(*font, hpText.c_str(), Vector2{renderPosition.x + 18.f, renderPosition.y + size_.y - 38.f}, 14.f, 1.f, WHITE);

    if (model_.block > 0) {
        const std::string blockText = model_.blockLabel + ": " + std::to_string(model_.block);
        DrawTextEx(*font, blockText.c_str(), Vector2{renderPosition.x + 14.f, renderPosition.y + 44.f}, 15.f, 1.f, UiTheme::toneText(UiTheme::Tone::Info));
    }

    float statusY = renderPosition.y + 84.f;
    if (model_.maxStress > 0) {
        std::string stressText = model_.stressLabel + ": " +
            std::to_string(std::max(0, model_.stress)) + "/" + std::to_string(model_.maxStress);
        if (!model_.stressBandName.empty()) {
            stressText += "  " + model_.stressBandName;
        }

        const Color stressColor = UiTheme::stressBand(model_.stressBandIndex);
        DrawTextEx(
            *font,
            UiUtf8::truncateWithEllipsis(stressText, 34).c_str(),
            Vector2{renderPosition.x + 14.f, renderPosition.y + 62.f},
            13.f,
            1.f,
            stressColor
        );

        const Rectangle stressBack{renderPosition.x + 14.f, renderPosition.y + 79.f, size_.x - 28.f, 8.f};
        const float stressRatio = static_cast<float>(std::clamp(model_.stress, 0, model_.maxStress)) /
            static_cast<float>(std::max(1, model_.maxStress));
        DrawRectangleRec(stressBack, UiTheme::panelInset);
        DrawRectangleRec(
            Rectangle{stressBack.x, stressBack.y, stressBack.width * stressRatio, stressBack.height},
            stressColor
        );

        for (const float thresholdRatio : {0.2f, 0.4f, 0.6f, 0.8f}) {
            const int markerX = static_cast<int>(stressBack.x + stressBack.width * thresholdRatio);
            DrawLine(markerX, static_cast<int>(stressBack.y), markerX, static_cast<int>(stressBack.y + stressBack.height), UiTheme::withAlpha(UiTheme::textPrimary, 0.58f));
        }
        statusY = renderPosition.y + 94.f;
    }

    if (!model_.stressPowerDescription.empty()) {
        DrawTextEx(
            *font,
            UiUtf8::truncateWithEllipsis(model_.stressPowerDescription, 34).c_str(),
            Vector2{renderPosition.x + 14.f, statusY},
            12.f,
            1.f,
            UiTheme::toneText(UiTheme::Tone::Warning)
        );
        statusY += 16.f;
    }
    if (!model_.stressBandRiskDescription.empty()) {
        DrawTextEx(
            *font,
            UiUtf8::truncateWithEllipsis(model_.stressBandRiskDescription, 34).c_str(),
            Vector2{renderPosition.x + 14.f, statusY},
            11.f,
            1.f,
            UiTheme::toneText(UiTheme::Tone::Danger)
        );
        statusY += 15.f;
    }
    if (!model_.activeStanceName.empty()) {
        const Rectangle stanceBounds{renderPosition.x + 12.f, statusY, size_.x - 24.f, 30.f};
        DrawRectangleRounded(stanceBounds, 0.35f, 10, UiTheme::toneFill(UiTheme::Tone::Primary));
        DrawRectangleRoundedLinesEx(stanceBounds, 0.35f, 10, 1.5f, UiTheme::toneBorder(UiTheme::Tone::Primary));

        const std::string stanceText = model_.activeStanceLabel + ": " + model_.activeStanceName;
        DrawTextEx(
            *font,
            UiUtf8::truncateWithEllipsis(stanceText, 24).c_str(),
            Vector2{stanceBounds.x + 9.f, stanceBounds.y + 7.f},
            13.f,
            1.f,
            UiTheme::toneText(UiTheme::Tone::Primary)
        );

        statusY += 36.f;

        if (!model_.stanceShiftBonusLabel.empty()) {
            DrawTextEx(
                *font,
                UiUtf8::truncateWithEllipsis(model_.stanceShiftBonusLabel, 34).c_str(),
                Vector2{renderPosition.x + 14.f, statusY},
                12.f,
                1.f,
                UiTheme::textSecondary
            );
            statusY += 18.f;
        }
    }

    (void)statusY;
    for (std::size_t i = 0; i < model_.statuses.size(); ++i) {
        const std::optional<Rectangle> chipBounds = statusBounds(i);
        if (!chipBounds.has_value()) {
            continue;
        }

        const StatusViewModel& status = model_.statuses[i];
        DrawRectangleRounded(*chipBounds, 0.35f, 8, UiTheme::statusFill(status));
        DrawRectangleRoundedLinesEx(*chipBounds, 0.35f, 8, 1.4f, UiTheme::statusBorder(status));

        const std::string text = statusChipText(status);
        DrawTextEx(
            *font,
            text.c_str(),
            Vector2{chipBounds->x + 8.f, chipBounds->y + 5.f},
            12.f,
            1.f,
            UiTheme::textPrimary
        );
    }
}

Rectangle PlayerView::bounds() const {
    return Rectangle{position_.x, position_.y, size_.x, size_.y};
}

Rectangle PlayerView::statusAreaBounds() const {
    const Vector2 renderPosition{position_.x + model_.renderOffset.x, position_.y + model_.renderOffset.y};
    constexpr float gap = 10.f;
    constexpr float columnWidth = 112.f;

    const float x = statusesOnRight_
        ? renderPosition.x + size_.x + gap
        : renderPosition.x - columnWidth - gap;

    return Rectangle{
        x,
        renderPosition.y + 10.f,
        columnWidth,
        std::max(24.f, size_.y - 20.f)
    };
}

std::optional<Rectangle> PlayerView::statusBounds(const std::size_t index) const {
    if (index >= model_.statuses.size()) {
        return std::nullopt;
    }

    const Rectangle area = statusAreaBounds();
    constexpr float chipHeight = 24.f;
    constexpr float gap = 6.f;

    const float x = area.x;
    float y = area.y;
    for (std::size_t i = 0; i <= index && i < model_.statuses.size(); ++i) {
        const Rectangle bounds{x, y, area.width, chipHeight};
        if (i == index) {
            if (bounds.y + bounds.height > area.y + area.height) {
                return std::nullopt;
            }
            return bounds;
        }

        y += chipHeight + gap;
    }

    return std::nullopt;
}

std::optional<std::size_t> PlayerView::statusIndexAt(const Vector2 worldPosition) const {
    for (std::size_t i = 0; i < model_.statuses.size(); ++i) {
        const std::optional<Rectangle> bounds = statusBounds(i);
        if (bounds.has_value() && CheckCollisionPointRec(worldPosition, *bounds)) {
            return i;
        }
    }

    return std::nullopt;
}

std::size_t PlayerView::visibleRelicSlotCount() const {
    return std::min(model_.relics.size(), maxVisibleRelicSlots);
}

std::optional<Rectangle> PlayerView::relicBounds(const std::size_t index) const {
    if (index >= visibleRelicSlotCount()) {
        return std::nullopt;
    }

    const Vector2 renderPosition{position_.x + model_.renderOffset.x, position_.y + model_.renderOffset.y};
    const std::size_t columns = size_.x >= 205.f ? 2u : 1u;
    const float totalGap = relicChipGap * static_cast<float>(columns - 1u);
    const float chipWidth = (size_.x - 28.f - totalGap) / static_cast<float>(columns);
    const std::size_t row = index / columns;
    const std::size_t column = index % columns;

    return Rectangle{
        renderPosition.x + 14.f + static_cast<float>(column) * (chipWidth + relicChipGap),
        renderPosition.y + size_.y + 74.f + static_cast<float>(row) * (relicChipHeight + relicChipGap),
        chipWidth,
        relicChipHeight
    };
}

std::optional<std::size_t> PlayerView::relicIndexAt(const Vector2 worldPosition) const {
    const std::size_t visibleCount = visibleRelicSlotCount();
    for (std::size_t i = 0; i < visibleCount; ++i) {
        if (relicSlotIsOverflow(i, model_.relics.size())) {
            continue;
        }

        const std::optional<Rectangle> bounds = relicBounds(i);
        if (bounds.has_value() && CheckCollisionPointRec(worldPosition, *bounds)) {
            return i;
        }
    }

    return std::nullopt;
}
