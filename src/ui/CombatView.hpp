#pragma once

#include "cards/CardInstanceId.hpp"
#include "entities/EntityId.hpp"
#include "ui/CombatViewModel.hpp"
#include "ui/EnemyView.hpp"
#include "ui/HandView.hpp"
#include "ui/PlayerView.hpp"

#include <raylib.h>

#include <cstddef>
#include <optional>
#include <vector>

class CombatView {
public:
    void setModel(const CombatViewModel& model);
    const CombatViewModel& model() const;

    void setSelectedCard(std::optional<CardInstanceId> selectedCardId);
    void setDraggedCard(std::optional<CardInstanceId> draggedCardId, Vector2 dragPosition);

    void update(float deltaSeconds, Vector2 mousePosition);
    void render(const Font* font) const;

    std::optional<CardInstanceId> hoveredCardId() const;
    std::optional<Vector2> cardCenter(CardInstanceId cardId) const;
    std::optional<EntityId> hoveredEnemyId() const;
    std::optional<Rectangle> hoveredEnemyBounds() const;
    std::optional<Rectangle> enemyBounds(EntityId entityId) const;
    std::optional<EntityId> hoveredPlayerId() const;
    std::optional<Rectangle> hoveredPlayerBounds() const;
    std::optional<Rectangle> playerBounds(EntityId entityId) const;
    std::optional<std::size_t> hoveredRelicIndex() const;
    std::optional<std::size_t> hoveredConsumableIndex() const;
    std::optional<std::size_t> hoveredDroneSlotIndex() const;
    std::optional<Rectangle> hoveredDroneSlotBounds() const;

    bool endTurnButtonContains(Vector2 mousePosition) const;

private:
    void applyResponsiveLayout();
    void layoutPlayers();
    void layoutEnemies();
    void renderDronePanel(const Font* font) const;

    Rectangle contentBounds() const;
    Rectangle battlefieldBounds() const;
    Rectangle handBounds() const;
    Rectangle endTurnButtonBounds() const;
    Rectangle relicBounds(std::size_t index) const;
    Rectangle consumableBounds(std::size_t index) const;
    Rectangle droneSlotBounds(std::size_t index) const;

private:
    CombatViewModel model_;
    HandView handView_;
    std::vector<PlayerView> playerViews_;
    std::vector<EnemyView> enemyViews_;

    std::optional<EntityId> hoveredEnemyId_;
    std::optional<EntityId> hoveredPlayerId_;
    std::optional<std::size_t> hoveredRelicIndex_;
    std::optional<std::size_t> hoveredConsumableIndex_;
    std::optional<std::size_t> hoveredDroneSlotIndex_;
    bool hoveredEndTurnButton_ = false;
    std::optional<CardInstanceId> draggedCardId_;
    Vector2 dragPosition_{0.f, 0.f};
};
