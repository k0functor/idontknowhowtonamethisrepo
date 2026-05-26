#include "CombatView.hpp"

#include <algorithm>
#include <string>

namespace {
std::string phaseText(const CombatPhase phase) {
    switch (phase) {
        case CombatPhase::NotStarted:
            return "Not started";
        case CombatPhase::PlayerTurn:
            return "Player turn";
        case CombatPhase::EnemyTurn:
            return "Enemy turn";
        case CombatPhase::Won:
            return "Victory";
        case CombatPhase::Lost:
            return "Defeat";
    }

    return "Unknown";
}

std::string shorten(const std::string& text, const std::size_t maxLength) {
    if (text.size() <= maxLength) {
        return text;
    }

    if (maxLength <= 3) {
        return text.substr(0, maxLength);
    }

    return text.substr(0, maxLength - 3) + "...";
}
}

void CombatView::setModel(const CombatViewModel& model) {
    model_ = model;
    handView_.setCards(model_.handCards);

    playerViews_.resize(model_.players.size());
    for (std::size_t i = 0; i < model_.players.size(); ++i) {
        playerViews_[i].setModel(model_.players[i]);
    }

    enemyViews_.resize(model_.enemies.size());
    for (std::size_t i = 0; i < model_.enemies.size(); ++i) {
        enemyViews_[i].setModel(model_.enemies[i]);
    }

    applyResponsiveLayout();
}

const CombatViewModel& CombatView::model() const {
    return model_;
}

void CombatView::setSelectedCard(const std::optional<CardInstanceId> selectedCardId) {
    handView_.setSelectedCard(selectedCardId);
}

void CombatView::setDraggedCard(
    const std::optional<CardInstanceId> draggedCardId,
    const Vector2 dragPosition
) {
    draggedCardId_ = draggedCardId;
    dragPosition_ = dragPosition;
    handView_.setDraggedCard(draggedCardId, dragPosition);
}

void CombatView::update(const float deltaSeconds, const Vector2 mousePosition) {
    applyResponsiveLayout();

    handView_.setDraggedCard(draggedCardId_, dragPosition_);
    handView_.update(deltaSeconds, mousePosition);

    hoveredEndTurnButton_ = endTurnButtonContains(mousePosition);

    hoveredRelicIndex_.reset();
    for (std::size_t i = 0; i < model_.relics.size(); ++i) {
        if (CheckCollisionPointRec(mousePosition, relicBounds(i))) {
            hoveredRelicIndex_ = i;
            break;
        }
    }

    hoveredConsumableIndex_.reset();
    for (std::size_t i = 0; i < model_.consumables.size(); ++i) {
        if (CheckCollisionPointRec(mousePosition, consumableBounds(i))) {
            hoveredConsumableIndex_ = i;
            break;
        }
    }

    hoveredPlayerId_.reset();
    for (const PlayerView& playerView : playerViews_) {
        if (playerView.contains(mousePosition)) {
            hoveredPlayerId_ = playerView.model().entityId;
            break;
        }
    }

    hoveredEnemyId_.reset();
    for (const EnemyView& enemyView : enemyViews_) {
        if (enemyView.contains(mousePosition)) {
            hoveredEnemyId_ = enemyView.model().entityId;
            break;
        }
    }
}

void CombatView::render(const Font* font) const {
    const int screenWidth = GetScreenWidth();
    const int screenHeight = GetScreenHeight();

    DrawRectangle(0, 0, screenWidth, screenHeight, Color{20, 20, 24, 255});
    DrawRectangle(0, 0, screenWidth, 72, Color{26, 28, 36, 255});

    const Rectangle battlefield = battlefieldBounds();
    const Rectangle handArea = handBounds();
    DrawRectangleLinesEx(battlefield, 1.f, Color{56, 58, 70, 255});
    DrawRectangleLinesEx(handArea, 1.f, Color{56, 58, 70, 255});

    const Rectangle endTurnBounds = endTurnButtonBounds();
    const Color endTurnColor = model_.canEndTurn
        ? (hoveredEndTurnButton_ ? Color{210, 170, 80, 255} : Color{160, 120, 50, 255})
        : Color{70, 70, 76, 255};
    DrawRectangleRounded(endTurnBounds, 0.18f, 8, endTurnColor);
    DrawRectangleRoundedLinesEx(endTurnBounds, 0.18f, 8, 2.f, Color{235, 220, 180, 255});

    if (font != nullptr) {
        const std::string energyText =
            model_.turnLabel + ": " + std::to_string(model_.turn) +
            "   " + model_.phaseText +
            "   " + model_.totalEnergyLabel + ": " + std::to_string(model_.energy) + "/" + std::to_string(model_.maxEnergy) +
            "   " + model_.drawPileLabel + ": " + std::to_string(model_.drawPileSize) +
            "   " + model_.discardPileLabel + ": " + std::to_string(model_.discardPileSize) +
            "   " + model_.exhaustPileLabel + ": " + std::to_string(model_.exhaustPileSize);

        DrawTextEx(*font, energyText.c_str(), Vector2{24.f, 20.f}, 18.f, 1.f, WHITE);
        DrawTextEx(*font, model_.endTurnLabel.c_str(), Vector2{endTurnBounds.x + 24.f, endTurnBounds.y + 18.f}, 18.f, 1.f, WHITE);

        for (std::size_t i = 0; i < model_.relics.size(); ++i) {
            const RelicViewModel& relic = model_.relics[i];
            const Rectangle bounds = relicBounds(i);
            const bool hovered = hoveredRelicIndex_.has_value() && *hoveredRelicIndex_ == i;
            const Color fill = hovered ? Color{95, 72, 38, 255} : Color{70, 58, 35, 255};
            const Color border = hovered ? Color{255, 225, 120, 255} : Color{230, 190, 90, 255};

            DrawRectangleRounded(bounds, 0.22f, 6, fill);
            DrawRectangleRoundedLinesEx(bounds, 0.22f, 6, hovered ? 2.5f : 1.5f, border);
            DrawTextEx(*font, shorten(relic.name, 15).c_str(), Vector2{bounds.x + 8.f, bounds.y + 5.f}, 13.f, 1.f, Color{230, 220, 180, 255});
        }

        for (std::size_t i = 0; i < model_.consumables.size(); ++i) {
            const ConsumableViewModel& consumable = model_.consumables[i];
            const Rectangle bounds = consumableBounds(i);
            const bool hovered = hoveredConsumableIndex_.has_value() && *hoveredConsumableIndex_ == i;
            const Color fill = !consumable.filled
                ? Color{38, 40, 48, 255}
                : (hovered ? Color{50, 82, 104, 255} : Color{38, 62, 82, 255});
            const Color border = hovered ? Color{130, 220, 255, 255} : Color{105, 150, 190, 255};

            DrawRectangleRounded(bounds, 0.22f, 6, fill);
            DrawRectangleRoundedLinesEx(bounds, 0.22f, 6, hovered ? 2.5f : 1.5f, border);

            const std::string text = consumable.filled ? shorten(consumable.name, 12) : model_.emptyLabel;
            DrawTextEx(*font, text.c_str(), Vector2{bounds.x + 8.f, bounds.y + 5.f}, 13.f, 1.f, Color{220, 235, 245, 255});
        }

        float logY = battlefield.y + 10.f;
        const float logX = battlefield.x + 14.f;
        for (const std::string& entry : model_.recentLogEntries) {
            DrawTextEx(*font, entry.c_str(), Vector2{logX, logY}, 13.f, 1.f, Color{190, 190, 200, 255});
            logY += 18.f;
        }
    }

    for (const PlayerView& playerView : playerViews_) {
        const bool hovered = hoveredPlayerId_.has_value() && playerView.model().entityId == *hoveredPlayerId_;
        playerView.render(font, hovered);
    }

    for (const EnemyView& enemyView : enemyViews_) {
        const bool hovered = hoveredEnemyId_.has_value() && enemyView.model().entityId == *hoveredEnemyId_;
        enemyView.render(font, hovered);
    }

    renderDronePanel(font);
    handView_.render(font);
}

std::optional<CardInstanceId> CombatView::hoveredCardId() const {
    return handView_.hoveredCardId();
}

std::optional<EntityId> CombatView::hoveredEnemyId() const {
    return hoveredEnemyId_;
}

std::optional<Rectangle> CombatView::hoveredEnemyBounds() const {
    if (!hoveredEnemyId_.has_value()) {
        return std::nullopt;
    }

    const Rectangle battlefield = battlefieldBounds();
    const float viewWidth = std::clamp(battlefield.width * 0.20f, 190.f, 250.f);
    const float viewHeight = 180.f;
    const float spacingX = viewWidth + 28.f;

    const float totalWidth = enemyViews_.empty()
        ? 0.f
        : viewWidth * static_cast<float>(enemyViews_.size()) + 28.f * static_cast<float>(enemyViews_.size() - 1);

    const float centerX = battlefield.x + battlefield.width * 0.70f;
    const float startX = centerX - totalWidth * 0.5f;
    const float y = battlefield.y + battlefield.height * 0.48f - viewHeight * 0.5f;

    for (std::size_t i = 0; i < enemyViews_.size(); ++i) {
        if (enemyViews_[i].model().entityId == *hoveredEnemyId_) {
            return Rectangle{
                startX + spacingX * static_cast<float>(i),
                y,
                viewWidth,
                viewHeight
            };
        }
    }

    return std::nullopt;
}

std::optional<EntityId> CombatView::hoveredPlayerId() const {
    return hoveredPlayerId_;
}

std::optional<Rectangle> CombatView::hoveredPlayerBounds() const {
    if (!hoveredPlayerId_.has_value()) {
        return std::nullopt;
    }

    const Rectangle battlefield = battlefieldBounds();
    const float viewWidth = std::clamp(
        battlefield.width * (playerViews_.size() > 1 ? 0.16f : 0.20f),
        160.f,
        235.f
    );
    const float viewHeight = 180.f;
    const float spacingX = viewWidth + 24.f;

    const float totalWidth = playerViews_.empty()
        ? 0.f
        : viewWidth * static_cast<float>(playerViews_.size()) + 24.f * static_cast<float>(playerViews_.size() - 1);

    const float centerX = battlefield.x + battlefield.width * 0.27f;
    const float startX = centerX - totalWidth * 0.5f;
    const float y = battlefield.y + battlefield.height * 0.48f - viewHeight * 0.5f;

    for (std::size_t i = 0; i < playerViews_.size(); ++i) {
        if (playerViews_[i].model().entityId == *hoveredPlayerId_) {
            return Rectangle{
                startX + spacingX * static_cast<float>(i),
                y,
                viewWidth,
                viewHeight
            };
        }
    }

    return std::nullopt;
}

std::optional<std::size_t> CombatView::hoveredRelicIndex() const {
    return hoveredRelicIndex_;
}

std::optional<std::size_t> CombatView::hoveredConsumableIndex() const {
    return hoveredConsumableIndex_;
}

bool CombatView::endTurnButtonContains(const Vector2 mousePosition) const {
    return model_.canEndTurn && CheckCollisionPointRec(mousePosition, endTurnButtonBounds());
}

void CombatView::applyResponsiveLayout() {
    handView_.setViewport(
        static_cast<float>(GetScreenWidth()),
        static_cast<float>(GetScreenHeight())
    );
    layoutPlayers();
    layoutEnemies();
}

void CombatView::layoutPlayers() {
    const Rectangle battlefield = battlefieldBounds();
    const float viewWidth = std::clamp(battlefield.width * (playerViews_.size() > 1 ? 0.16f : 0.20f), 160.f, 235.f);
    const float viewHeight = 180.f;
    const float spacingX = viewWidth + 24.f;

    const float totalWidth = playerViews_.empty()
        ? 0.f
        : viewWidth * static_cast<float>(playerViews_.size()) + 24.f * static_cast<float>(playerViews_.size() - 1);

    const float centerX = battlefield.x + battlefield.width * 0.27f;
    const float startX = centerX - totalWidth * 0.5f;
    const float y = battlefield.y + battlefield.height * 0.48f - viewHeight * 0.5f;

    for (std::size_t i = 0; i < playerViews_.size(); ++i) {
        playerViews_[i].setSize(Vector2{viewWidth, viewHeight});
        playerViews_[i].setPosition(Vector2{startX + spacingX * static_cast<float>(i), y});
    }
}

void CombatView::layoutEnemies() {
    const Rectangle battlefield = battlefieldBounds();
    const float viewWidth = std::clamp(battlefield.width * 0.20f, 190.f, 250.f);
    const float viewHeight = 180.f;
    const float spacingX = viewWidth + 28.f;

    const float totalWidth = enemyViews_.empty()
        ? 0.f
        : viewWidth * static_cast<float>(enemyViews_.size()) + 28.f * static_cast<float>(enemyViews_.size() - 1);
    const float centerX = battlefield.x + battlefield.width * 0.70f;
    const float startX = centerX - totalWidth * 0.5f;
    const float y = battlefield.y + battlefield.height * 0.48f - viewHeight * 0.5f;

    for (std::size_t i = 0; i < enemyViews_.size(); ++i) {
        enemyViews_[i].setPosition(Vector2{startX + spacingX * static_cast<float>(i), y});
    }
}

void CombatView::renderDronePanel(const Font* font) const {
    if (model_.droneSlots.empty() || font == nullptr) {
        return;
    }

    const float slotWidth = 118.f;
    const float slotHeight = 44.f;
    const float gap = 10.f;
    const float totalWidth = slotWidth * static_cast<float>(model_.droneSlots.size()) + gap * static_cast<float>(model_.droneSlots.size() - 1);
    const float x = static_cast<float>(GetScreenWidth()) * 0.5f - totalWidth * 0.5f;
    const float y = 82.f;

    DrawTextEx(*font, model_.droneSlotsLabel.c_str(), Vector2{x, y - 22.f}, 14.f, 1.f, Color{170, 220, 230, 255});

    for (std::size_t i = 0; i < model_.droneSlots.size(); ++i) {
        const DroneSlotViewModel& slot = model_.droneSlots[i];
        const Rectangle bounds = droneSlotBounds(i);
        const Color fill = slot.filled ? Color{38, 68, 78, 255} : Color{34, 36, 44, 255};
        const Color border = slot.filled ? Color{120, 220, 235, 255} : Color{85, 95, 110, 255};

        DrawRectangleRounded(bounds, 0.18f, 8, fill);
        DrawRectangleRoundedLinesEx(bounds, 0.18f, 8, 2.f, border);
        DrawTextEx(*font, shorten(slot.name, 13).c_str(), Vector2{bounds.x + 8.f, bounds.y + 13.f}, 14.f, 1.f, Color{220, 245, 250, 255});
    }
}

Rectangle CombatView::battlefieldBounds() const {
    const float margin = 14.f;
    const float top = 82.f;
    const float handHeight = std::clamp(static_cast<float>(GetScreenHeight()) * 0.36f, 250.f, 330.f);
    const float bottom = static_cast<float>(GetScreenHeight()) - handHeight - 10.f;
    return Rectangle{
        margin,
        top,
        static_cast<float>(GetScreenWidth()) - margin * 2.f,
        std::max(220.f, bottom - top)
    };
}

Rectangle CombatView::handBounds() const {
    const float margin = 14.f;
    const float handHeight = std::clamp(static_cast<float>(GetScreenHeight()) * 0.36f, 250.f, 330.f);
    return Rectangle{
        margin,
        static_cast<float>(GetScreenHeight()) - handHeight,
        static_cast<float>(GetScreenWidth()) - margin * 2.f,
        handHeight - 10.f
    };
}

Rectangle CombatView::endTurnButtonBounds() const {
    const Rectangle hand = handBounds();
    const float width = 150.f;
    const float height = 58.f;
    return Rectangle{
        hand.x + hand.width - width - 24.f,
        hand.y - height - 16.f,
        width,
        height
    };
}

Rectangle CombatView::relicBounds(const std::size_t index) const {
    const float x = 24.f + static_cast<float>(index) * 148.f;
    return Rectangle{x, 44.f, 136.f, 24.f};
}

Rectangle CombatView::consumableBounds(const std::size_t index) const {
    const float width = 136.f;
    const float height = 24.f;
    const float x = static_cast<float>(GetScreenWidth()) - 24.f - width - static_cast<float>(index) * 148.f;
    return Rectangle{x, 44.f, width, height};
}

Rectangle CombatView::droneSlotBounds(const std::size_t index) const {
    const float slotWidth = 118.f;
    const float slotHeight = 44.f;
    const float gap = 10.f;
    const float totalWidth = slotWidth * static_cast<float>(model_.droneSlots.size()) + gap * static_cast<float>(model_.droneSlots.size() - 1);
    const float x = static_cast<float>(GetScreenWidth()) * 0.5f - totalWidth * 0.5f + static_cast<float>(index) * (slotWidth + gap);
    return Rectangle{x, 82.f, slotWidth, slotHeight};
}
