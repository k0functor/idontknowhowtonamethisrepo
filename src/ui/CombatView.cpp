#include "CombatView.hpp"

#include <string>

void CombatView::setModel(const CombatViewModel& model) {
    model_ = model;
    handView_.setCards(model_.handCards);

    enemyViews_.resize(model_.enemies.size());
    for (std::size_t i = 0; i < model_.enemies.size(); ++i) {
        enemyViews_[i].setModel(model_.enemies[i]);
    }

    layoutEnemies();
}

void CombatView::setSelectedCard(const std::optional<CardInstanceId> selectedCardId) {
    handView_.setSelectedCard(selectedCardId);
}

void CombatView::update(const float deltaSeconds, const Vector2 mousePosition) {
    handView_.update(deltaSeconds, mousePosition);

    hoveredEnemyId_.reset();
    for (const EnemyView& enemyView : enemyViews_) {
        if (enemyView.contains(mousePosition)) {
            hoveredEnemyId_ = enemyView.model().entityId;
            break;
        }
    }
}

void CombatView::render(const Font* font) const {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{20, 20, 24, 255});
    DrawRectangle(0, 0, GetScreenWidth(), 72, Color{26, 28, 36, 255});
    DrawRectangleLinesEx(Rectangle{10.f, 82.f, static_cast<float>(GetScreenWidth()) - 20.f, 388.f}, 1.f, Color{56, 58, 70, 255});
    DrawRectangleLinesEx(Rectangle{10.f, 480.f, static_cast<float>(GetScreenWidth()) - 20.f, 230.f}, 1.f, Color{56, 58, 70, 255});

    if (font != nullptr) {
        const std::string energyText =
            "Energy: " + std::to_string(model_.energy) + "/" + std::to_string(model_.maxEnergy) +
            "   Draw: " + std::to_string(model_.drawPileSize) +
            "   Discard: " + std::to_string(model_.discardPileSize) +
            "   Exhaust: " + std::to_string(model_.exhaustPileSize);

        DrawTextEx(*font, energyText.c_str(), Vector2{24.f, 20.f}, 18.f, 1.f, WHITE);

        float logY = 92.f;
        for (const std::string& entry : model_.recentLogEntries) {
            DrawTextEx(*font, entry.c_str(), Vector2{24.f, logY}, 13.f, 1.f, Color{190, 190, 200, 255});
            logY += 18.f;
        }
    }

    for (const EnemyView& enemyView : enemyViews_) {
        const bool hovered = hoveredEnemyId_.has_value() && enemyView.model().entityId == *hoveredEnemyId_;
        enemyView.render(font, hovered);
    }

    handView_.render(font);
}

std::optional<CardInstanceId> CombatView::hoveredCardId() const {
    return handView_.hoveredCardId();
}

std::optional<EntityId> CombatView::hoveredEnemyId() const {
    return hoveredEnemyId_;
}

void CombatView::layoutEnemies() {
    const float startX = 850.f;
    const float startY = 210.f;
    const float spacingX = 260.f;

    for (std::size_t i = 0; i < enemyViews_.size(); ++i) {
        enemyViews_[i].setPosition(Vector2{startX + spacingX * static_cast<float>(i), startY});
    }
}
