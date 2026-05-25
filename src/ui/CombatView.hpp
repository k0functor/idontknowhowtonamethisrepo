#pragma once

#include "ui/CombatViewModel.hpp"
#include "ui/EnemyView.hpp"
#include "ui/HandView.hpp"

#include <raylib.h>

#include <optional>
#include <vector>

class CombatView {
public:
    void setModel(const CombatViewModel& model);
    void setSelectedCard(std::optional<CardInstanceId> selectedCardId);

    void update(float deltaSeconds, Vector2 mousePosition);
    void render(const Font* font) const;

    std::optional<CardInstanceId> hoveredCardId() const;
    std::optional<EntityId> hoveredEnemyId() const;

    bool endTurnButtonContains(Vector2 mousePosition) const;

private:
    void layoutEnemies();
    Rectangle endTurnButtonBounds() const;

private:
    CombatViewModel model_;
    HandView handView_;
    std::vector<EnemyView> enemyViews_;

    std::optional<EntityId> hoveredEnemyId_;
    bool hoveredEndTurnButton_ = false;
};
