#pragma once

#include "ui/CombatViewModel.hpp"
#include "ui/EnemyView.hpp"
#include "ui/HandView.hpp"
#include "ui/PlayerView.hpp"

#include <raylib.h>

#include <optional>
#include <vector>

class CombatView {
public:
    void setModel(const CombatViewModel& model);
    void setSelectedCard(std::optional<CardInstanceId> selectedCardId);
    void setDraggedCard(std::optional<CardInstanceId> draggedCardId, Vector2 dragPosition);

    void update(float deltaSeconds, Vector2 mousePosition);
    void render(const Font* font) const;

    std::optional<CardInstanceId> hoveredCardId() const;
    std::optional<EntityId> hoveredEnemyId() const;
    std::optional<EntityId> hoveredPlayerId() const;

    bool endTurnButtonContains(Vector2 mousePosition) const;

private:
    void applyResponsiveLayout();
    void layoutPlayers();
    void layoutEnemies();
    Rectangle battlefieldBounds() const;
    Rectangle handBounds() const;
    Rectangle endTurnButtonBounds() const;

private:
    CombatViewModel model_;
    HandView handView_;
    std::vector<PlayerView> playerViews_;
    std::vector<EnemyView> enemyViews_;

    std::optional<EntityId> hoveredEnemyId_;
    std::optional<EntityId> hoveredPlayerId_;
    bool hoveredEndTurnButton_ = false;
    std::optional<CardInstanceId> draggedCardId_;
    Vector2 dragPosition_{0.f, 0.f};
};
