#include "PlayerView.hpp"

#include "ui/Utf8.hpp"

#include <algorithm>
#include <sstream>
#include <string>

namespace {
Color statusFillColor(const StatusViewModel& status) {
    if (status.debuff) {
        return Color{94, 46, 54, 238};
    }
    if (status.buff) {
        return Color{42, 82, 60, 238};
    }
    return Color{58, 62, 74, 238};
}

Color statusBorderColor(const StatusViewModel& status) {
    if (status.debuff) {
        return Color{236, 112, 126, 255};
    }
    if (status.buff) {
        return Color{120, 226, 154, 255};
    }
    return Color{176, 184, 208, 255};
}

std::string statusChipText(const StatusViewModel& status) {
    std::string text = UiUtf8::truncateWithEllipsis(status.name, 8);
    if (status.amount > 0) {
        text += " " + std::to_string(status.amount);
    }
    return text;
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
    const Color fill = model_.alive ? Color{58, 82, 92, 255} : Color{52, 52, 52, 255};
    const Color outline = model_.previewTarget
        ? Color{255, 218, 90, 255}
        : (model_.targetable
            ? Color{115, 235, 165, 255}
            : (model_.activeTurn
                ? Color{255, 205, 70, 255}
                : (hovered ? Color{255, 230, 130, 255} : Color{200, 220, 235, 255})));
    const float outlineThickness = model_.previewTarget ? 5.f : (model_.targetable ? 3.5f : (model_.activeTurn ? 4.5f : (hovered ? 4.f : 2.f)));

    DrawRectangleRounded(body, 0.12f, 12, fill);
    DrawRectangleRoundedLinesEx(body, 0.12f, 12, outlineThickness, outline);

    const float hpRatio = model_.maxHp <= 0
        ? 0.f
        : static_cast<float>(std::max(0, model_.currentHp)) / static_cast<float>(model_.maxHp);

    const Rectangle hpBack{renderPosition.x + 14.f, renderPosition.y + size_.y - 36.f, size_.x - 28.f, 18.f};
    DrawRectangleRec(hpBack, Color{30, 34, 38, 255});

    const Rectangle hpFill{hpBack.x, hpBack.y, hpBack.width * hpRatio, hpBack.height};
    DrawRectangleRec(hpFill, Color{70, 165, 120, 255});

    if (font == nullptr) {
        return;
    }

    DrawTextEx(*font, model_.name.c_str(), Vector2{renderPosition.x + 14.f, renderPosition.y + 12.f}, 20.f, 1.f, WHITE);

    if (model_.activeTurn) {
        DrawTextEx(*font, model_.activeTurnLabel.c_str(), Vector2{renderPosition.x + size_.x - 78.f, renderPosition.y + 14.f}, 14.f, 1.f, Color{255, 222, 110, 255});
    }

    const std::string hpText = std::to_string(model_.currentHp) + "/" + std::to_string(model_.maxHp);
    DrawTextEx(*font, hpText.c_str(), Vector2{renderPosition.x + 18.f, renderPosition.y + size_.y - 38.f}, 14.f, 1.f, WHITE);

    if (model_.block > 0) {
        const std::string blockText = model_.blockLabel + ": " + std::to_string(model_.block);
        DrawTextEx(*font, blockText.c_str(), Vector2{renderPosition.x + 14.f, renderPosition.y + 44.f}, 15.f, 1.f, Color{180, 220, 255, 255});
    }

    if (model_.maxStress > 0) {
        const std::string stressText = model_.stressLabel + ": " +
            std::to_string(std::max(0, model_.stress)) + "/" + std::to_string(model_.maxStress);
        DrawTextEx(*font, stressText.c_str(), Vector2{renderPosition.x + 14.f, renderPosition.y + 62.f}, 14.f, 1.f, Color{220, 185, 230, 255});
    }

    float statusY = renderPosition.y + 84.f;
    if (!model_.activeStanceName.empty()) {
        const Rectangle stanceBounds{renderPosition.x + 12.f, statusY, size_.x - 24.f, 30.f};
        DrawRectangleRounded(stanceBounds, 0.35f, 10, Color{72, 48, 96, 235});
        DrawRectangleRoundedLinesEx(stanceBounds, 0.35f, 10, 1.5f, Color{185, 132, 235, 255});

        const std::string stanceText = model_.activeStanceLabel + ": " + model_.activeStanceName;
        DrawTextEx(
            *font,
            UiUtf8::truncateWithEllipsis(stanceText, 24).c_str(),
            Vector2{stanceBounds.x + 9.f, stanceBounds.y + 7.f},
            13.f,
            1.f,
            Color{238, 222, 255, 255}
        );

        statusY += 36.f;

        if (!model_.stanceShiftBonusLabel.empty()) {
            DrawTextEx(
                *font,
                UiUtf8::truncateWithEllipsis(model_.stanceShiftBonusLabel, 34).c_str(),
                Vector2{renderPosition.x + 14.f, statusY},
                12.f,
                1.f,
                Color{204, 184, 226, 255}
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
        DrawRectangleRounded(*chipBounds, 0.35f, 8, statusFillColor(status));
        DrawRectangleRoundedLinesEx(*chipBounds, 0.35f, 8, 1.4f, statusBorderColor(status));

        const std::string text = statusChipText(status);
        DrawTextEx(
            *font,
            text.c_str(),
            Vector2{chipBounds->x + 8.f, chipBounds->y + 5.f},
            12.f,
            1.f,
            Color{238, 242, 232, 255}
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
