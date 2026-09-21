#include "CombatSceneLayout.hpp"

#include "ui/CardVisualInstance.hpp"
#include "ui/VirtualViewport.hpp"

#include <algorithm>

namespace {
Vector2 rectangleCenter(const Rectangle bounds) {
    return Vector2{bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f};
}
}

Vector2 CombatSceneLayout::playedCardCenterPosition() {
    return Vector2{
        static_cast<float>(VirtualViewport::width()) * 0.5f,
        static_cast<float>(VirtualViewport::height()) * 0.47f
    };
}

Vector2 CombatSceneLayout::playedCardQueuePosition(const std::size_t queueIndex) {
    const Vector2 center = playedCardCenterPosition();
    const float offset = static_cast<float>(queueIndex - 1u);
    return Vector2{
        center.x - 250.f - offset * 42.f,
        center.y + 18.f + offset * 20.f
    };
}

Vector2 CombatSceneLayout::discardPileCenterPosition() {
    return rectangleCenter(discardPileButtonBounds());
}

Rectangle CombatSceneLayout::drawPileButtonBounds() {
    constexpr float width = 126.f;
    constexpr float height = 34.f;
    constexpr float margin = 24.f;
    return Rectangle{
        margin,
        static_cast<float>(VirtualViewport::height()) - height - margin,
        width,
        height
    };
}

Rectangle CombatSceneLayout::discardPileButtonBounds() {
    constexpr float width = 126.f;
    constexpr float height = 34.f;
    constexpr float margin = 24.f;
    constexpr float gap = 12.f;
    return Rectangle{
        static_cast<float>(VirtualViewport::width()) - margin - width * 2.f - gap,
        static_cast<float>(VirtualViewport::height()) - height - margin,
        width,
        height
    };
}

Rectangle CombatSceneLayout::exhaustPileButtonBounds() {
    const Rectangle discard = discardPileButtonBounds();
    constexpr float gap = 12.f;
    return Rectangle{discard.x + discard.width + gap, discard.y, discard.width, discard.height};
}

Rectangle CombatSceneLayout::energyBubbleBounds(const std::optional<Rectangle> primaryPlayerBounds) {
    constexpr float size = 62.f;
    constexpr float gapBelowActor = 12.f;
    constexpr float gapAboveHand = 10.f;
    const float screenHeight = static_cast<float>(VirtualViewport::height());
    const float handHeight = std::clamp(screenHeight * 0.36f, 250.f, 330.f);
    const float handTop = screenHeight - handHeight;

    if (primaryPlayerBounds.has_value()) {
        const float centerX = primaryPlayerBounds->x + primaryPlayerBounds->width * 0.5f;
        const float preferredY = primaryPlayerBounds->y + primaryPlayerBounds->height + gapBelowActor;
        const float maxY = handTop - size - gapAboveHand;
        return Rectangle{
            centerX - size * 0.5f,
            std::min(preferredY, maxY),
            size,
            size
        };
    }

    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - size * 0.5f,
        handTop - size - gapAboveHand,
        size,
        size
    };
}

Rectangle CombatSceneLayout::pileOverlayBounds() {
    const float width = std::min(1180.f, static_cast<float>(VirtualViewport::width()) - 56.f);
    const float height = std::min(680.f, static_cast<float>(VirtualViewport::height()) - 56.f);
    return Rectangle{
        (static_cast<float>(VirtualViewport::width()) - width) * 0.5f,
        (static_cast<float>(VirtualViewport::height()) - height) * 0.5f,
        width,
        height
    };
}

Rectangle CombatSceneLayout::pileOverlayGridBounds(const Rectangle modal) {
    return Rectangle{modal.x + 28.f, modal.y + 86.f, modal.width - 56.f, modal.height - 158.f};
}

Rectangle CombatSceneLayout::pileOverlayCloseButtonBounds(const Rectangle modal) {
    return Rectangle{modal.x + modal.width - 146.f, modal.y + modal.height - 58.f, 112.f, 40.f};
}

Rectangle CombatSceneLayout::pileOverlayCardBounds(
    const Rectangle grid,
    const std::size_t index,
    const float scrollOffset
) {
    constexpr int columns = 5;
    constexpr float gap = 20.f;
    const Vector2 cardSize = CardVisualInstance::standardDisplaySize();
    const float width = cardSize.x + 18.f;
    const float height = cardSize.y + 26.f;
    const float totalWidth = static_cast<float>(columns) * width + static_cast<float>(columns - 1) * gap;
    const float startX = grid.x + std::max(0.f, (grid.width - totalWidth) * 0.5f);
    const int column = static_cast<int>(index % columns);
    const int row = static_cast<int>(index / columns);
    return Rectangle{
        startX + static_cast<float>(column) * (width + gap),
        grid.y + 14.f + static_cast<float>(row) * (height + gap) - scrollOffset,
        width,
        height
    };
}

float CombatSceneLayout::pileOverlayMaxScroll(const Rectangle grid, const std::size_t count) {
    if (count == 0) {
        return 0.f;
    }

    constexpr int columns = 5;
    constexpr float gap = 20.f;
    const Vector2 cardSize = CardVisualInstance::standardDisplaySize();
    const float height = cardSize.y + 26.f;
    const std::size_t rows = (count + columns - 1) / columns;
    const float totalHeight = 28.f + static_cast<float>(rows) * height +
        static_cast<float>(rows > 0 ? rows - 1 : 0) * gap;
    return std::max(0.f, totalHeight - grid.height);
}

Rectangle CombatSceneLayout::combatItemInspectModalBounds() {
    const float screenWidth = static_cast<float>(VirtualViewport::width());
    const float screenHeight = static_cast<float>(VirtualViewport::height());
    const float width = std::min(620.f, screenWidth - 80.f);
    const float height = std::min(560.f, screenHeight - 80.f);
    return Rectangle{
        (screenWidth - width) * 0.5f,
        (screenHeight - height) * 0.5f,
        width,
        height
    };
}

Rectangle CombatSceneLayout::combatItemInspectCloseButtonBounds(const Rectangle modal) {
    return Rectangle{modal.x + modal.width - 136.f, modal.y + modal.height - 58.f, 112.f, 40.f};
}

Rectangle CombatSceneLayout::combatItemInspectPreviousButtonBounds(const Rectangle modal) {
    return Rectangle{modal.x + 24.f, modal.y + modal.height - 58.f, 52.f, 40.f};
}

Rectangle CombatSceneLayout::combatItemInspectNextButtonBounds(const Rectangle modal) {
    return Rectangle{modal.x + 86.f, modal.y + modal.height - 58.f, 52.f, 40.f};
}

Rectangle CombatSceneLayout::consumableConfirmationBounds() {
    const float width = std::min(560.f, static_cast<float>(VirtualViewport::width()) - 72.f);
    constexpr float height = 340.f;
    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - width * 0.5f,
        static_cast<float>(VirtualViewport::height()) * 0.5f - height * 0.5f,
        width,
        height
    };
}

Rectangle CombatSceneLayout::consumableConfirmButtonBounds(const Rectangle modal) {
    return Rectangle{modal.x + modal.width * 0.5f + 18.f, modal.y + modal.height - 70.f, 190.f, 48.f};
}

Rectangle CombatSceneLayout::consumableCancelButtonBounds(const Rectangle modal) {
    return Rectangle{modal.x + modal.width * 0.5f - 208.f, modal.y + modal.height - 70.f, 190.f, 48.f};
}

Rectangle CombatSceneLayout::rewardModalBounds() {
    const float width = std::min(620.f, static_cast<float>(VirtualViewport::width()) - 72.f);
    const float height = std::min(460.f, static_cast<float>(VirtualViewport::height()) - 72.f);
    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - width * 0.5f,
        static_cast<float>(VirtualViewport::height()) * 0.5f - height * 0.5f,
        width,
        height
    };
}

Rectangle CombatSceneLayout::rewardOptionRowBounds(const std::size_t index) {
    const Rectangle panel = rewardModalBounds();
    return Rectangle{
        panel.x + 38.f,
        panel.y + 112.f + static_cast<float>(index) * 78.f,
        panel.width - 76.f,
        62.f
    };
}

Rectangle CombatSceneLayout::rewardContinueButtonBounds() {
    const Rectangle panel = rewardModalBounds();
    return Rectangle{panel.x + panel.width * 0.5f - 160.f, panel.y + panel.height - 68.f, 320.f, 50.f};
}

Rectangle CombatSceneLayout::rewardCardChoiceModalBounds() {
    const float width = std::min(1040.f, static_cast<float>(VirtualViewport::width()) - 72.f);
    const float height = std::min(560.f, static_cast<float>(VirtualViewport::height()) - 72.f);
    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - width * 0.5f,
        static_cast<float>(VirtualViewport::height()) * 0.5f - height * 0.5f,
        width,
        height
    };
}

Rectangle CombatSceneLayout::rewardCardChoiceOptionBounds(
    const std::size_t optionCount,
    const std::size_t index
) {
    const Rectangle panel = rewardCardChoiceModalBounds();
    const std::size_t visibleCount = std::min<std::size_t>(3u, optionCount);
    const Vector2 cardSize = CardVisualInstance::standardDisplaySize();
    const float cardWidth = cardSize.x + 18.f;
    const float cardHeight = cardSize.y + 26.f;
    if (visibleCount == 0) {
        return Rectangle{panel.x + 40.f, panel.y + 100.f, cardWidth, cardHeight};
    }

    constexpr float spacing = 34.f;
    const float totalWidth = cardWidth * static_cast<float>(visibleCount) +
        spacing * static_cast<float>(visibleCount - 1u);
    const float startX = panel.x + panel.width * 0.5f - totalWidth * 0.5f;
    return Rectangle{
        startX + static_cast<float>(index) * (cardWidth + spacing),
        panel.y + 90.f,
        cardWidth,
        cardHeight
    };
}

Rectangle CombatSceneLayout::rewardCardChoiceCancelBounds() {
    const Rectangle panel = rewardCardChoiceModalBounds();
    return Rectangle{panel.x + panel.width * 0.5f - 250.f, panel.y + panel.height - 66.f, 220.f, 48.f};
}

Rectangle CombatSceneLayout::rewardCardChoiceConfirmBounds() {
    const Rectangle panel = rewardCardChoiceModalBounds();
    return Rectangle{panel.x + panel.width * 0.5f + 30.f, panel.y + panel.height - 66.f, 220.f, 48.f};
}

Rectangle CombatSceneLayout::fallbackFeedbackTargetBounds() {
    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - 80.f,
        static_cast<float>(VirtualViewport::height()) * 0.5f - 60.f,
        160.f,
        120.f
    };
}

Vector2 CombatSceneLayout::feedbackAnchor(const Rectangle targetBounds) {
    return Vector2{
        targetBounds.x + targetBounds.width * 0.5f,
        targetBounds.y + targetBounds.height * 0.18f
    };
}
