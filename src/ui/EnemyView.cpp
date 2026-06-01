#include "EnemyView.hpp"

#include <algorithm>
#include <sstream>

#include "ui/Utf8.hpp"

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

namespace {
Color withAlpha(const Color color, const float opacity) {
    const float clamped = std::clamp(opacity, 0.f, 1.f);
    return Color{
        color.r,
        color.g,
        color.b,
        static_cast<unsigned char>(static_cast<float>(color.a) * clamped)
    };
}
}

void EnemyView::setModel(EnemyViewModel model) {
    model_ = std::move(model);
}

const EnemyViewModel& EnemyView::model() const {
    return model_;
}

void EnemyView::setPosition(const Vector2 position) {
    position_ = position;
}

bool EnemyView::contains(const Vector2 worldPosition) const {
    return CheckCollisionPointRec(worldPosition, bounds());
}

void EnemyView::render(const Font* font, const bool hovered) const {
    if (model_.opacity <= 0.02f) {
        return;
    }

    const Vector2 renderPosition{position_.x + model_.renderOffset.x, position_.y + model_.renderOffset.y};
    const Rectangle body{renderPosition.x, renderPosition.y, size_.x, size_.y};
    const Color fill = model_.alive ? Color{82, 58, 58, 255} : Color{52, 52, 52, 255};
    const Color outline = model_.previewTarget
        ? Color{255, 218, 90, 255}
        : (model_.targetable
            ? Color{115, 235, 165, 255}
            : (hovered ? Color{255, 220, 120, 255} : Color{210, 210, 210, 255}));
    const float outlineThickness = model_.previewTarget ? 5.f : (model_.targetable ? 3.5f : (hovered ? 4.f : 2.f));

    DrawRectangleRec(body, withAlpha(fill, model_.opacity));
    DrawRectangleLinesEx(body, outlineThickness, withAlpha(outline, model_.opacity));

    const float hpRatio = model_.maxHp <= 0
        ? 0.f
        : static_cast<float>(std::max(0, model_.currentHp)) / static_cast<float>(model_.maxHp);

    const Rectangle hpBack{renderPosition.x + 14.f, renderPosition.y + size_.y - 36.f, size_.x - 28.f, 18.f};
    DrawRectangleRec(hpBack, withAlpha(Color{32, 32, 36, 255}, model_.opacity));

    const Rectangle hpFill{hpBack.x, hpBack.y, hpBack.width * hpRatio, hpBack.height};
    DrawRectangleRec(hpFill, withAlpha(Color{160, 60, 60, 255}, model_.opacity));

    if (font == nullptr) {
        return;
    }

    DrawTextEx(*font, model_.name.c_str(), Vector2{renderPosition.x + 14.f, renderPosition.y + 12.f}, 20.f, 1.f, withAlpha(WHITE, model_.opacity));

    if (!model_.intentText.empty()) {
        const Color intentColor = model_.intent.type == EnemyIntentType::Attack
            ? Color{255, 160, 140, 255}
            : Color{180, 215, 255, 255};

        DrawTextEx(*font, model_.intentText.c_str(), Vector2{renderPosition.x + 14.f, renderPosition.y + 42.f}, 16.f, 1.f, withAlpha(intentColor, model_.opacity));
    }

    const std::string hpText = std::to_string(model_.currentHp) + "/" + std::to_string(model_.maxHp);
    DrawTextEx(*font, hpText.c_str(), Vector2{renderPosition.x + 18.f, renderPosition.y + size_.y - 38.f}, 14.f, 1.f, withAlpha(WHITE, model_.opacity));

    if (model_.block > 0) {
        const std::string blockText = model_.blockLabel + ": " + std::to_string(model_.block);
        DrawTextEx(*font, blockText.c_str(), Vector2{renderPosition.x + 14.f, renderPosition.y + 68.f}, 15.f, 1.f, withAlpha(Color{180, 215, 255, 255}, model_.opacity));
    }

    for (std::size_t i = 0; i < model_.statuses.size(); ++i) {
        const std::optional<Rectangle> chipBounds = statusBounds(i);
        if (!chipBounds.has_value()) {
            continue;
        }

        const StatusViewModel& status = model_.statuses[i];
        DrawRectangleRounded(*chipBounds, 0.35f, 8, withAlpha(statusFillColor(status), model_.opacity));
        DrawRectangleRoundedLinesEx(*chipBounds, 0.35f, 8, 1.4f, withAlpha(statusBorderColor(status), model_.opacity));

        const std::string text = statusChipText(status);
        DrawTextEx(
            *font,
            text.c_str(),
            Vector2{chipBounds->x + 8.f, chipBounds->y + 5.f},
            12.f,
            1.f,
            withAlpha(Color{238, 242, 232, 255}, model_.opacity)
        );
    }
}

Rectangle EnemyView::bounds() const {
    return Rectangle{position_.x, position_.y, size_.x, size_.y};
}

Rectangle EnemyView::statusAreaBounds() const {
    const Vector2 renderPosition{position_.x + model_.renderOffset.x, position_.y + model_.renderOffset.y};
    return Rectangle{renderPosition.x + 12.f, renderPosition.y + size_.y - 70.f, size_.x - 24.f, 28.f};
}

std::optional<Rectangle> EnemyView::statusBounds(const std::size_t index) const {
    if (index >= model_.statuses.size()) {
        return std::nullopt;
    }

    const Rectangle area = statusAreaBounds();
    constexpr float chipHeight = 24.f;
    constexpr float gap = 6.f;
    constexpr float minChipWidth = 42.f;
    constexpr float maxChipWidth = 94.f;

    float x = area.x;
    float y = area.y;
    for (std::size_t i = 0; i <= index && i < model_.statuses.size(); ++i) {
        const StatusViewModel& status = model_.statuses[i];
        const float width = std::clamp(
            26.f + static_cast<float>(UiUtf8::truncateWithEllipsis(status.name, 8).size()) * 6.5f +
                (status.amount > 0 ? 20.f : 0.f),
            minChipWidth,
            maxChipWidth
        );

        if (x + width > area.x + area.width && x > area.x) {
            x = area.x;
            y += chipHeight + gap;
        }

        const Rectangle bounds{x, y, width, chipHeight};
        if (i == index) {
            if (bounds.y + bounds.height > area.y + 2.f * (chipHeight + gap)) {
                return std::nullopt;
            }
            return bounds;
        }

        x += width + gap;
    }

    return std::nullopt;
}

std::optional<std::size_t> EnemyView::statusIndexAt(const Vector2 worldPosition) const {
    for (std::size_t i = 0; i < model_.statuses.size(); ++i) {
        const std::optional<Rectangle> bounds = statusBounds(i);
        if (bounds.has_value() && CheckCollisionPointRec(worldPosition, *bounds)) {
            return i;
        }
    }

    return std::nullopt;
}
