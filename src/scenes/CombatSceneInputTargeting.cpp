#include "CombatScene.hpp"

#include "cards/CardDefinition.hpp"
#include "cards/CardUpgrade.hpp"
#include "combat/CombatPhase.hpp"
#include "effects/EffectDefinition.hpp"
#include "effects/EffectTarget.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <vector>

namespace {
Vector2 rectangleCenter(const Rectangle bounds) {
    return Vector2{bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f};
}

Vector2 arrowTipOnBounds(const Rectangle bounds, const Vector2 from) {
    const Vector2 center = rectangleCenter(bounds);
    const float halfWidth = bounds.width * 0.5f;
    const float halfHeight = bounds.height * 0.5f;
    const float dx = center.x - from.x;
    const float dy = center.y - from.y;

    if (std::abs(dx) < 0.001f && std::abs(dy) < 0.001f) {
        return center;
    }

    const float scaleX = std::abs(dx) > 0.001f ? halfWidth / std::abs(dx) : 100000.f;
    const float scaleY = std::abs(dy) > 0.001f ? halfHeight / std::abs(dy) : 100000.f;
    const float scale = std::min(scaleX, scaleY);

    return Vector2{center.x - dx * scale, center.y - dy * scale};
}

void drawTargetArrow(const Vector2 from, const Rectangle targetBounds) {
    const Vector2 tip = arrowTipOnBounds(targetBounds, from);
    const float dx = tip.x - from.x;
    const float dy = tip.y - from.y;
    const float length = std::sqrt(dx * dx + dy * dy);

    if (length < 8.f) {
        return;
    }

    const Vector2 direction{dx / length, dy / length};
    const Vector2 normal{-direction.y, direction.x};
    const Vector2 lineEnd{tip.x - direction.x * 18.f, tip.y - direction.y * 18.f};

    const Color lineColor{255, 215, 92, 225};
    const Color shadowColor{0, 0, 0, 125};

    DrawLineEx(Vector2{from.x + 2.f, from.y + 2.f}, Vector2{lineEnd.x + 2.f, lineEnd.y + 2.f}, 7.f, shadowColor);
    DrawLineEx(from, lineEnd, 5.f, lineColor);

    const Vector2 left{
        tip.x - direction.x * 24.f + normal.x * 10.f,
        tip.y - direction.y * 24.f + normal.y * 10.f
    };
    const Vector2 right{
        tip.x - direction.x * 24.f - normal.x * 10.f,
        tip.y - direction.y * 24.f - normal.y * 10.f
    };

    DrawTriangle(Vector2{tip.x + 2.f, tip.y + 2.f}, Vector2{left.x + 2.f, left.y + 2.f}, Vector2{right.x + 2.f, right.y + 2.f}, shadowColor);
    DrawTriangle(tip, left, right, lineColor);

    DrawRectangleRoundedLinesEx(targetBounds, 0.08f, 10, 4.f, Color{255, 218, 90, 230});
}
}

void CombatScene::handleKeyboardCombatInput() {
    if (state_.phase != CombatPhase::PlayerTurn) {
        return;
    }

    if (IsKeyPressed(KEY_RIGHT)) {
        selectCardByOffset(1);
    }

    if (IsKeyPressed(KEY_LEFT)) {
        selectCardByOffset(-1);
    }

    if (selectedCardId_.has_value()) {
        if (IsKeyPressed(KEY_D)) {
            cycleKeyboardTarget(1);
        }

        if (IsKeyPressed(KEY_A)) {
            cycleKeyboardTarget(-1);
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            ensureKeyboardTargetForSelectedCard();
            const std::optional<EntityId> target = previewTargetForSelectedCard();
            if (target.has_value()) {
                playSelectedCardOn(*target);
            }
        }
    }
}

void CombatScene::selectCardByOffset(const int offset) {
    const std::vector<CardInstanceId> ids = handCardIds();
    if (ids.empty()) {
        clearCardSelection();
        return;
    }

    std::size_t index = 0;
    if (selectedCardId_.has_value()) {
        if (const std::optional<std::size_t> current = handCardIndex(*selectedCardId_)) {
            index = *current;
        }
    } else if (offset < 0) {
        index = ids.size() - 1;
    }

    if (selectedCardId_.has_value()) {
        const int size = static_cast<int>(ids.size());
        const int current = static_cast<int>(index);
        index = static_cast<std::size_t>((current + offset + size) % size);
    }

    selectCard(ids[index]);
}

void CombatScene::selectCard(const CardInstanceId cardInstanceId) {
    if (!state_.hand.contains(cardInstanceId)) {
        clearCardSelection();
        return;
    }

    selectedCardId_ = cardInstanceId;
    draggedCardId_.reset();
    inspectedCardId_.reset();
    ensureKeyboardTargetForSelectedCard();
    viewModelDirty_ = true;
}

void CombatScene::cycleKeyboardTarget(const int offset) {
    if (!selectedCardId_.has_value()) {
        keyboardTargetId_.reset();
        return;
    }

    const std::vector<EntityId> candidates = targetCandidatesForCard(*selectedCardId_);
    if (candidates.empty()) {
        keyboardTargetId_.reset();
        return;
    }

    std::size_t index = 0;
    if (keyboardTargetId_.has_value()) {
        const auto iterator = std::find(candidates.begin(), candidates.end(), *keyboardTargetId_);
        if (iterator != candidates.end()) {
            index = static_cast<std::size_t>(std::distance(candidates.begin(), iterator));
        }
    }

    const int size = static_cast<int>(candidates.size());
    const int current = static_cast<int>(index);
    index = static_cast<std::size_t>((current + offset + size) % size);
    keyboardTargetId_ = candidates[index];
    lastPreviewTarget_.reset();
    viewModelDirty_ = true;
}

void CombatScene::ensureKeyboardTargetForSelectedCard() {
    if (!selectedCardId_.has_value()) {
        keyboardTargetId_.reset();
        return;
    }

    const std::vector<EntityId> candidates = targetCandidatesForCard(*selectedCardId_);
    if (candidates.empty()) {
        keyboardTargetId_.reset();
        return;
    }

    if (keyboardTargetId_.has_value() &&
        std::find(candidates.begin(), candidates.end(), *keyboardTargetId_) != candidates.end()) {
        return;
    }

    keyboardTargetId_ = candidates.front();
}

void CombatScene::clearCardSelection() {
    pendingConsumableIndex_.reset();
    selectedCardId_.reset();
    draggedCardId_.reset();
    keyboardTargetId_.reset();
    lastPreviewTarget_.reset();
    viewModelDirty_ = true;
}

std::vector<CardInstanceId> CombatScene::handCardIds() const {
    std::vector<CardInstanceId> result;
    result.reserve(state_.hand.cards().size());

    for (const CardInstance& card : state_.hand.cards()) {
        result.push_back(card.instanceId);
    }

    return result;
}

std::optional<std::size_t> CombatScene::handCardIndex(const CardInstanceId cardInstanceId) const {
    const std::vector<CardInstanceId> ids = handCardIds();
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (ids[i] == cardInstanceId) {
            return i;
        }
    }

    return std::nullopt;
}

std::vector<EntityId> CombatScene::targetCandidatesForCard(const CardInstanceId cardInstanceId) const {
    if (!state_.hand.contains(cardInstanceId)) {
        return {};
    }

    if (cardCanTargetEnemy(cardInstanceId)) {
        return state_.aliveEnemyIds();
    }

    if (cardCanTargetPlayer(cardInstanceId)) {
        const CardInstance& instance = state_.hand.get(cardInstanceId);
        const CardDefinition definition = CardUpgrade::effectiveDefinition(content_.cards().get(instance.definitionId), instance.upgraded);
        const EntityId source = sourceForCard(cardInstanceId);

        bool hasAllyTarget = false;
        bool hasAllAlliesTarget = false;
        for (const EffectDefinition& effect : definition.effects) {
            if (effect.target == EffectTarget::Ally) {
                hasAllyTarget = true;
            }
            if (effect.target == EffectTarget::AllAllies || effect.target == EffectTarget::RandomAlly) {
                hasAllAlliesTarget = true;
            }
        }

        if (hasAllyTarget || hasAllAlliesTarget) {
            return state_.alivePlayerIds();
        }

        if (state_.hasEntity(source) && state_.isPlayer(source)) {
            return {source};
        }

        return state_.alivePlayerIds();
    }

    const EntityId source = sourceForCard(cardInstanceId);
    if (state_.hasEntity(source)) {
        return {source};
    }

    return {};
}

std::optional<EntityId> CombatScene::preferredTargetForCard(const CardInstanceId cardInstanceId) const {
    const std::vector<EntityId> candidates = targetCandidatesForCard(cardInstanceId);
    if (candidates.empty()) {
        return std::nullopt;
    }

    return candidates.front();
}

std::optional<EntityId> CombatScene::previewTargetForSelectedCard() const {
    if (!selectedCardId_.has_value()) {
        return std::nullopt;
    }

    if (view_.hoveredEnemyId().has_value() && selectedCardCanTargetEnemy()) {
        return view_.hoveredEnemyId();
    }

    if (view_.hoveredPlayerId().has_value() && selectedCardCanTargetPlayer()) {
        return view_.hoveredPlayerId();
    }

    if (keyboardTargetId_.has_value()) {
        const std::vector<EntityId> candidates = targetCandidatesForCard(*selectedCardId_);
        if (std::find(candidates.begin(), candidates.end(), *keyboardTargetId_) != candidates.end()) {
            return keyboardTargetId_;
        }
    }

    return preferredTargetForCard(*selectedCardId_);
}

std::optional<EntityId> CombatScene::arrowTargetForCard(const CardInstanceId cardInstanceId) const {
    if (!state_.hand.contains(cardInstanceId)) {
        return std::nullopt;
    }

    if (selectedCardId_.has_value() && cardInstanceId == *selectedCardId_) {
        return previewTargetForSelectedCard();
    }

    if (view_.hoveredEnemyId().has_value() && cardCanTargetEnemy(cardInstanceId)) {
        return view_.hoveredEnemyId();
    }

    if (view_.hoveredPlayerId().has_value() && cardCanTargetPlayer(cardInstanceId)) {
        return view_.hoveredPlayerId();
    }

    return std::nullopt;
}

void CombatScene::renderTargetingArrow() const {
    std::optional<CardInstanceId> sourceCardId;
    if (selectedCardId_.has_value()) {
        sourceCardId = selectedCardId_;
    } else if (view_.hoveredCardId().has_value()) {
        sourceCardId = view_.hoveredCardId();
    }

    if (!sourceCardId.has_value()) {
        return;
    }

    const std::optional<EntityId> target = arrowTargetForCard(*sourceCardId);
    if (!target.has_value()) {
        return;
    }

    const std::optional<Vector2> sourceCenter = view_.cardCenter(*sourceCardId);
    if (!sourceCenter.has_value()) {
        return;
    }

    std::optional<Rectangle> targetBounds;
    if (state_.isEnemy(*target)) {
        targetBounds = view_.enemyBounds(*target);
    } else if (state_.isPlayer(*target)) {
        targetBounds = view_.playerBounds(*target);
    }

    if (!targetBounds.has_value()) {
        return;
    }

    drawTargetArrow(*sourceCenter, *targetBounds);
}

void CombatScene::handleMousePressed(const Vector2 mousePosition) {
    if (view_.hoveredRelicIndex().has_value()) {
        openRelicInspect(*view_.hoveredRelicIndex());
        return;
    }

    if (view_.hoveredConsumableIndex().has_value()) {
        openConsumableConfirmation(*view_.hoveredConsumableIndex());
        return;
    }

    if (view_.endTurnButtonContains(mousePosition)) {
        endPlayerTurn();
        return;
    }

    if (view_.hoveredCardId().has_value()) {
        selectCard(*view_.hoveredCardId());
        draggedCardId_ = selectedCardId_;
        viewModelDirty_ = true;
        return;
    }

    if (selectedCardId_.has_value() && view_.hoveredEnemyId().has_value() && selectedCardCanTargetEnemy()) {
        playSelectedCardOn(*view_.hoveredEnemyId());
        return;
    }

    if (selectedCardId_.has_value() && view_.hoveredPlayerId().has_value() && selectedCardCanTargetPlayer()) {
        playSelectedCardOn(*view_.hoveredPlayerId());
        return;
    }

    clearCardSelection();
}

void CombatScene::handleMouseReleased(const Vector2) {
    if (!draggedCardId_.has_value()) {
        return;
    }

    if (view_.hoveredEnemyId().has_value() && selectedCardCanTargetEnemy()) {
        playSelectedCardOn(*view_.hoveredEnemyId());
        return;
    }

    if (view_.hoveredPlayerId().has_value() && selectedCardCanTargetPlayer()) {
        playSelectedCardOn(*view_.hoveredPlayerId());
        return;
    }

    draggedCardId_.reset();
    ensureKeyboardTargetForSelectedCard();
    lastPreviewTarget_.reset();
    viewModelDirty_ = true;
}

bool CombatScene::selectedCardCanTargetEnemy() const {
    return selectedCardId_.has_value() && cardCanTargetEnemy(*selectedCardId_);
}

bool CombatScene::selectedCardCanTargetPlayer() const {
    return selectedCardId_.has_value() && cardCanTargetPlayer(*selectedCardId_);
}

bool CombatScene::cardCanTargetEnemy(const CardInstanceId cardInstanceId) const {
    if (!state_.hand.contains(cardInstanceId)) {
        return false;
    }

    const CardInstance& instance = state_.hand.get(cardInstanceId);
    const CardDefinition definition = CardUpgrade::effectiveDefinition(content_.cards().get(instance.definitionId), instance.upgraded);

    for (const EffectDefinition& effect : definition.effects) {
        switch (effect.target) {
            case EffectTarget::SingleEnemy:
            case EffectTarget::AllEnemies:
            case EffectTarget::RandomEnemy:
                return true;
            default:
                break;
        }
    }

    return false;
}

bool CombatScene::cardCanTargetPlayer(const CardInstanceId cardInstanceId) const {
    if (!state_.hand.contains(cardInstanceId)) {
        return false;
    }

    const CardInstance& instance = state_.hand.get(cardInstanceId);
    const CardDefinition definition = CardUpgrade::effectiveDefinition(content_.cards().get(instance.definitionId), instance.upgraded);

    bool hasPlayerTarget = false;

    for (const EffectDefinition& effect : definition.effects) {
        switch (effect.target) {
            case EffectTarget::Self:
            case EffectTarget::Ally:
            case EffectTarget::AllAllies:
            case EffectTarget::RandomAlly:
                hasPlayerTarget = true;
                break;
            case EffectTarget::SingleEnemy:
            case EffectTarget::AllEnemies:
            case EffectTarget::RandomEnemy:
                return false;
        }
    }

    return hasPlayerTarget;
}
