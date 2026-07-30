#include "EnemyView.hpp"

#include <algorithm>
#include <cmath>
#include <string>

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

Color withAlpha(const Color color, const float opacity) {
    const float clamped = std::clamp(opacity, 0.f, 1.f);
    return Color{
        color.r,
        color.g,
        color.b,
        static_cast<unsigned char>(static_cast<float>(color.a) * clamped)
    };
}

Color intentFillColor(const EnemyIntentType type) {
    switch (type) {
        case EnemyIntentType::Attack:
            return Color{102, 43, 43, 250};
        case EnemyIntentType::Block:
            return Color{39, 69, 101, 250};
        case EnemyIntentType::Buff:
            return Color{42, 82, 59, 250};
        case EnemyIntentType::Debuff:
            return Color{86, 49, 96, 250};
        case EnemyIntentType::Special:
            return Color{94, 73, 38, 250};
        case EnemyIntentType::Unknown:
            return Color{58, 60, 69, 250};
    }

    return Color{58, 60, 69, 250};
}

Color intentBorderColor(const EnemyIntentType type) {
    switch (type) {
        case EnemyIntentType::Attack:
            return Color{255, 135, 122, 255};
        case EnemyIntentType::Block:
            return Color{128, 202, 255, 255};
        case EnemyIntentType::Buff:
            return Color{125, 232, 156, 255};
        case EnemyIntentType::Debuff:
            return Color{220, 150, 240, 255};
        case EnemyIntentType::Special:
            return Color{255, 218, 116, 255};
        case EnemyIntentType::Unknown:
            return Color{190, 194, 205, 255};
    }

    return Color{190, 194, 205, 255};
}

void drawCornerReticle(const Rectangle bounds, const Color color, const float thickness) {
    constexpr float length = 22.f;
    DrawLineEx(Vector2{bounds.x, bounds.y}, Vector2{bounds.x + length, bounds.y}, thickness, color);
    DrawLineEx(Vector2{bounds.x, bounds.y}, Vector2{bounds.x, bounds.y + length}, thickness, color);
    DrawLineEx(Vector2{bounds.x + bounds.width, bounds.y}, Vector2{bounds.x + bounds.width - length, bounds.y}, thickness, color);
    DrawLineEx(Vector2{bounds.x + bounds.width, bounds.y}, Vector2{bounds.x + bounds.width, bounds.y + length}, thickness, color);
    DrawLineEx(Vector2{bounds.x, bounds.y + bounds.height}, Vector2{bounds.x + length, bounds.y + bounds.height}, thickness, color);
    DrawLineEx(Vector2{bounds.x, bounds.y + bounds.height}, Vector2{bounds.x, bounds.y + bounds.height - length}, thickness, color);
    DrawLineEx(Vector2{bounds.x + bounds.width, bounds.y + bounds.height}, Vector2{bounds.x + bounds.width - length, bounds.y + bounds.height}, thickness, color);
    DrawLineEx(Vector2{bounds.x + bounds.width, bounds.y + bounds.height}, Vector2{bounds.x + bounds.width, bounds.y + bounds.height - length}, thickness, color);
}
} // namespace

void EnemyView::setModel(EnemyViewModel model) {
    model_ = std::move(model);
}

const EnemyViewModel& EnemyView::model() const {
    return model_;
}

void EnemyView::setPosition(const Vector2 position) {
    position_ = position;
}

void EnemyView::setSize(const Vector2 size) {
    size_.x = std::max(120.f, size.x);
    size_.y = std::max(140.f, size.y);
}

void EnemyView::setStatusesOnRight(const bool statusesOnRight) {
    statusesOnRight_ = statusesOnRight;
}

void EnemyView::setCompactStatuses(const bool compactStatuses) {
    compactStatuses_ = compactStatuses;
}

bool EnemyView::contains(const Vector2 worldPosition) const {
    return CheckCollisionPointRec(worldPosition, bounds()) ||
           (model_.alive && CheckCollisionPointRec(worldPosition, intentBounds()));
}

void EnemyView::render(const Font* font, const bool hovered) const {
    if (model_.opacity <= 0.02f) {
        return;
    }

    const Rectangle body = bounds();
    const Rectangle intentPanel = intentBounds();
    const Color fill = model_.alive ? Color{82, 58, 58, 255} : Color{46, 46, 50, 255};
    const Color outline = model_.previewTarget
        ? Color{255, 218, 90, 255}
        : (model_.targetable
            ? Color{115, 235, 165, 255}
            : (hovered ? Color{255, 220, 120, 255} : Color{190, 194, 202, 255}));
    const float outlineThickness = model_.previewTarget ? 5.f : (model_.targetable ? 3.5f : (hovered ? 4.f : 2.f));

    DrawRectangleRounded(body, 0.08f, 8, withAlpha(fill, model_.opacity));
    DrawRectangleRoundedLinesEx(body, 0.08f, 8, outlineThickness, withAlpha(outline, model_.opacity));

    if (model_.targetable || model_.previewTarget) {
        const Rectangle reticle{body.x - 7.f, body.y - 7.f, body.width + 14.f, body.height + 14.f};
        drawCornerReticle(reticle, withAlpha(outline, model_.opacity), model_.previewTarget ? 4.f : 2.5f);
    }

    if (model_.alive) {
        const Color intentFill = intentFillColor(model_.intent.type);
        const Color intentBorder = intentBorderColor(model_.intent.type);
        DrawRectangleRounded(intentPanel, 0.22f, 8, withAlpha(intentFill, model_.opacity));
        DrawRectangleRoundedLinesEx(intentPanel, 0.22f, 8, 2.f, withAlpha(intentBorder, model_.opacity));

        if (model_.intentAffectsMultipleTargets && !model_.intentScopeLabel.empty()) {
            const float chipWidth = std::min(96.f, intentPanel.width * 0.44f);
            const Rectangle scopeChip{
                intentPanel.x + intentPanel.width - chipWidth - 6.f,
                intentPanel.y + 6.f,
                chipWidth,
                intentPanel.height - 12.f
            };
            DrawRectangleRounded(scopeChip, 0.3f, 6, withAlpha(Color{25, 27, 34, 210}, model_.opacity));
            DrawRectangleRoundedLinesEx(scopeChip, 0.3f, 6, 1.f, withAlpha(intentBorder, model_.opacity));
        }
    }

    const float hpRatio = model_.maxHp <= 0
        ? 0.f
        : static_cast<float>(std::max(0, model_.currentHp)) / static_cast<float>(model_.maxHp);

    const Rectangle hpBack{body.x + 14.f, body.y + body.height - 36.f, body.width - 28.f, 18.f};
    DrawRectangleRec(hpBack, withAlpha(Color{32, 32, 36, 255}, model_.opacity));
    DrawRectangleRec(Rectangle{hpBack.x, hpBack.y, hpBack.width * hpRatio, hpBack.height}, withAlpha(Color{160, 60, 60, 255}, model_.opacity));

    const Rectangle formationBadge{body.x + body.width - 47.f, body.y + 8.f, 38.f, 22.f};
    DrawRectangleRounded(formationBadge, 0.35f, 6, withAlpha(Color{29, 31, 38, 230}, model_.opacity));
    DrawRectangleRoundedLinesEx(formationBadge, 0.35f, 6, 1.f, withAlpha(Color{155, 162, 181, 255}, model_.opacity));

    if (font == nullptr) {
        return;
    }

    const float nameWidth = std::max(70.f, body.width - 76.f);
    const std::string displayName = UiUtf8::truncateWithEllipsis(model_.name, static_cast<std::size_t>(std::max(8.f, nameWidth / 10.f)));
    DrawTextEx(*font, displayName.c_str(), Vector2{body.x + 14.f, body.y + 12.f}, body.width < 175.f ? 17.f : 20.f, 1.f, withAlpha(WHITE, model_.opacity));
    DrawTextEx(*font, model_.formationLabel.c_str(), Vector2{formationBadge.x + 7.f, formationBadge.y + 4.f}, 12.f, 1.f, withAlpha(Color{218, 223, 234, 255}, model_.opacity));

    if (!model_.phaseName.empty() && model_.alive) {
        const std::string phaseText = UiUtf8::truncateWithEllipsis(model_.phaseName, static_cast<std::size_t>(std::max(8.f, body.width / 10.f)));
        const Rectangle phaseChip{body.x + 12.f, body.y + 39.f, body.width - 24.f, 22.f};
        DrawRectangleRounded(phaseChip, 0.3f, 6, withAlpha(Color{69, 49, 83, 225}, model_.opacity));
        DrawRectangleRoundedLinesEx(phaseChip, 0.3f, 6, 1.f, withAlpha(Color{209, 157, 238, 255}, model_.opacity));
        DrawTextEx(*font, phaseText.c_str(), Vector2{phaseChip.x + 8.f, phaseChip.y + 4.f}, 12.f, 1.f, withAlpha(Color{244, 222, 255, 255}, model_.opacity));
    }

    if (model_.alive) {
        const float scopeReservation = model_.intentAffectsMultipleTargets ? std::min(104.f, intentPanel.width * 0.47f) : 8.f;
        const std::size_t intentLimit = static_cast<std::size_t>(std::max(7.f, (intentPanel.width - scopeReservation - 18.f) / 8.5f));
        const std::string intentText = UiUtf8::truncateWithEllipsis(model_.intentText, intentLimit);
        DrawTextEx(*font, intentText.c_str(), Vector2{intentPanel.x + 10.f, intentPanel.y + 10.f}, 15.f, 1.f, withAlpha(intentBorderColor(model_.intent.type), model_.opacity));

        if (model_.intentAffectsMultipleTargets && !model_.intentScopeLabel.empty()) {
            const float chipWidth = std::min(96.f, intentPanel.width * 0.44f);
            const Rectangle scopeChip{intentPanel.x + intentPanel.width - chipWidth - 6.f, intentPanel.y + 6.f, chipWidth, intentPanel.height - 12.f};
            const std::string scope = UiUtf8::truncateWithEllipsis(model_.intentScopeLabel, 13);
            DrawTextEx(*font, scope.c_str(), Vector2{scopeChip.x + 7.f, scopeChip.y + 6.f}, 11.f, 1.f, withAlpha(Color{240, 241, 246, 255}, model_.opacity));
        }
    } else {
        const std::string label = model_.defeatedLabel.empty() ? "DEFEATED" : model_.defeatedLabel;
        const Vector2 labelSize = MeasureTextEx(*font, label.c_str(), 18.f, 1.f);
        DrawTextEx(
            *font,
            label.c_str(),
            Vector2{body.x + body.width * 0.5f - labelSize.x * 0.5f, body.y + body.height * 0.48f},
            18.f,
            1.f,
            withAlpha(Color{204, 204, 211, 255}, model_.opacity)
        );
    }

    const std::string hpText = std::to_string(model_.currentHp) + "/" + std::to_string(model_.maxHp);
    DrawTextEx(*font, hpText.c_str(), Vector2{body.x + 18.f, body.y + body.height - 38.f}, 14.f, 1.f, withAlpha(WHITE, model_.opacity));

    if (model_.block > 0) {
        const std::string blockText = model_.blockLabel + ": " + std::to_string(model_.block);
        DrawTextEx(*font, blockText.c_str(), Vector2{body.x + 14.f, body.y + (model_.phaseName.empty() ? 48.f : 66.f)}, 14.f, 1.f, withAlpha(Color{180, 215, 255, 255}, model_.opacity));
    }

    const std::size_t capacity = visibleStatusCapacity();
    const bool hasStatusOverflow = capacity > 0 && model_.statuses.size() > capacity;
    const std::size_t visibleCount = hasStatusOverflow
        ? capacity - 1
        : std::min(capacity, model_.statuses.size());
    for (std::size_t i = 0; i < visibleCount; ++i) {
        const std::optional<Rectangle> chipBounds = statusBounds(i);
        if (!chipBounds.has_value()) {
            continue;
        }

        const StatusViewModel& status = model_.statuses[i];
        DrawRectangleRounded(*chipBounds, 0.35f, 8, withAlpha(statusFillColor(status), model_.opacity));
        DrawRectangleRoundedLinesEx(*chipBounds, 0.35f, 8, 1.4f, withAlpha(statusBorderColor(status), model_.opacity));

        const std::string text = statusChipText(status);
        DrawTextEx(*font, text.c_str(), Vector2{chipBounds->x + 8.f, chipBounds->y + (compactStatuses_ ? 3.f : 5.f)}, compactStatuses_ ? 10.f : 12.f, 1.f, withAlpha(Color{238, 242, 232, 255}, model_.opacity));
    }

    if (hasStatusOverflow) {
        const std::size_t hidden = model_.statuses.size() - visibleCount;
        const Rectangle moreBounds = statusBounds(visibleCount).value_or(Rectangle{});
        DrawRectangleRounded(moreBounds, 0.35f, 6, withAlpha(Color{48, 50, 59, 245}, model_.opacity));
        DrawRectangleRoundedLinesEx(moreBounds, 0.35f, 6, 1.f, withAlpha(Color{180, 184, 198, 255}, model_.opacity));
        const std::string moreText = "+" + std::to_string(hidden);
        DrawTextEx(*font, moreText.c_str(), Vector2{moreBounds.x + 9.f, moreBounds.y + 3.f}, 11.f, 1.f, withAlpha(WHITE, model_.opacity));
    }
}

Rectangle EnemyView::bounds() const {
    return Rectangle{
        position_.x + model_.renderOffset.x,
        position_.y + model_.renderOffset.y,
        size_.x,
        size_.y
    };
}

Rectangle EnemyView::intentBounds() const {
    const Rectangle body = bounds();
    return Rectangle{body.x + 4.f, body.y - 48.f, body.width - 8.f, 40.f};
}

Rectangle EnemyView::statusAreaBounds() const {
    const Rectangle body = bounds();

    if (compactStatuses_) {
        return Rectangle{
            body.x + 10.f,
            body.y + 72.f,
            std::max(80.f, body.width - 20.f),
            std::max(40.f, body.height - 114.f)
        };
    }

    constexpr float gap = 10.f;
    constexpr float columnWidth = 112.f;
    const float x = statusesOnRight_ ? body.x + body.width + gap : body.x - columnWidth - gap;
    return Rectangle{x, body.y + 10.f, columnWidth, std::max(24.f, body.height - 20.f)};
}

std::size_t EnemyView::visibleStatusCapacity() const {
    const Rectangle area = statusAreaBounds();
    if (compactStatuses_) {
        constexpr float rowHeight = 22.f;
        const std::size_t rows = static_cast<std::size_t>(std::max(0.f, std::floor((area.height + 4.f) / rowHeight)));
        return rows * 2;
    }

    constexpr float rowHeight = 30.f;
    return static_cast<std::size_t>(std::max(0.f, std::floor((area.height + 6.f) / rowHeight)));
}

std::optional<Rectangle> EnemyView::statusBounds(const std::size_t index) const {
    if (index >= model_.statuses.size() || index >= visibleStatusCapacity()) {
        return std::nullopt;
    }

    const Rectangle area = statusAreaBounds();
    if (compactStatuses_) {
        constexpr std::size_t columns = 2;
        constexpr float chipHeight = 18.f;
        constexpr float gap = 4.f;
        const float chipWidth = std::max(36.f, (area.width - gap) * 0.5f);
        const std::size_t row = index / columns;
        const std::size_t column = index % columns;
        return Rectangle{
            area.x + static_cast<float>(column) * (chipWidth + gap),
            area.y + static_cast<float>(row) * (chipHeight + gap),
            chipWidth,
            chipHeight
        };
    }

    constexpr float chipHeight = 24.f;
    constexpr float gap = 6.f;
    return Rectangle{area.x, area.y + static_cast<float>(index) * (chipHeight + gap), area.width, chipHeight};
}

std::optional<std::size_t> EnemyView::statusIndexAt(const Vector2 worldPosition) const {
    const std::size_t capacity = visibleStatusCapacity();
    const std::size_t visibleCount = capacity > 0 && model_.statuses.size() > capacity
        ? capacity - 1
        : std::min(capacity, model_.statuses.size());
    for (std::size_t i = 0; i < visibleCount; ++i) {
        const std::optional<Rectangle> chipBounds = statusBounds(i);
        if (chipBounds.has_value() && CheckCollisionPointRec(worldPosition, *chipBounds)) {
            return i;
        }
    }

    return std::nullopt;
}
