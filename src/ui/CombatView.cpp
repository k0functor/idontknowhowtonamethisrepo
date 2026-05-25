#include "CombatView.hpp"

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
}

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

    hoveredEndTurnButton_ = endTurnButtonContains(mousePosition);

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

    const Rectangle endTurnBounds = endTurnButtonBounds();
    const Color endTurnColor = model_.canEndTurn
        ? (hoveredEndTurnButton_ ? Color{210, 170, 80, 255} : Color{160, 120, 50, 255})
        : Color{70, 70, 76, 255};
    DrawRectangleRec(endTurnBounds, endTurnColor);
    DrawRectangleLinesEx(endTurnBounds, 2.f, Color{235, 220, 180, 255});

    if (font != nullptr) {
        const std::string energyText =
            "Turn: " + std::to_string(model_.turn) +
            "   " + phaseText(model_.phase) +
            "   Energy: " + std::to_string(model_.energy) + "/" + std::to_string(model_.maxEnergy) +
            "   HP: " + std::to_string(model_.playerCurrentHp) + "/" + std::to_string(model_.playerMaxHp) +
            "   Block: " + std::to_string(model_.playerBlock) +
            "   Draw: " + std::to_string(model_.drawPileSize) +
            "   Discard: " + std::to_string(model_.discardPileSize) +
            "   Exhaust: " + std::to_string(model_.exhaustPileSize);

        DrawTextEx(*font, energyText.c_str(), Vector2{24.f, 20.f}, 18.f, 1.f, WHITE);
        DrawTextEx(*font, "End Turn", Vector2{endTurnBounds.x + 30.f, endTurnBounds.y + 18.f}, 18.f, 1.f, WHITE);

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

bool CombatView::endTurnButtonContains(const Vector2 mousePosition) const {
    return model_.canEndTurn && CheckCollisionPointRec(mousePosition, endTurnButtonBounds());
}

void CombatView::layoutEnemies() {
    const float startX = 850.f;
    const float startY = 210.f;
    const float spacingX = 260.f;

    for (std::size_t i = 0; i < enemyViews_.size(); ++i) {
        enemyViews_[i].setPosition(Vector2{startX + spacingX * static_cast<float>(i), startY});
    }
}

Rectangle CombatView::endTurnButtonBounds() const {
    return Rectangle{
        static_cast<float>(GetScreenWidth()) - 178.f,
        static_cast<float>(GetScreenHeight()) - 176.f,
        150.f,
        58.f
    };
}
