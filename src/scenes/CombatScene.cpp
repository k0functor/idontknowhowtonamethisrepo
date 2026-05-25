#include "CombatScene.hpp"

#include "cards/CardDefinition.hpp"
#include "combat/CombatPhase.hpp"
#include "effects/EffectTarget.hpp"
#include "enemies/EnemyInstance.hpp"
#include "ui/BasicUi.hpp"

#include <iostream>
#include <stdexcept>

namespace {
CombatEntity makeDebugPlayer(const EntityId id) {
    CombatEntity player;
    player.id = id;
    player.type = EntityType::Player;
    player.definitionId = "debug_player";
    player.nameTextId = TextId("debug.player.name");
    player.health = Health(70);
    player.block = 0;
    player.statuses.add("strength", 3);
    return player;
}
}

CombatScene::CombatScene(
    const ContentRegistry& content,
    const LocalizationManager& localization,
    Random& random,
    const UiFont& uiFont,
    const RunState& runState,
    std::function<void()> onCombatWon,
    std::function<void()> onCombatLost
)
    : content_(content),
      localization_(localization),
      random_(random),
      uiFont_(uiFont),
      runState_(runState),
      onCombatWon_(std::move(onCombatWon)),
      onCombatLost_(std::move(onCombatLost)),
      damageSystem_(modifierSystem_),
      blockSystem_(modifierSystem_),
      effectSystem_(
          effectResolver_,
          targeting_,
          damageSystem_,
          blockSystem_,
          energySystem_,
          drawSystem_
      ),
      cardPlaySystem_(
          content_.cards(),
          validator_,
          energySystem_,
          effectSystem_
      ),
      previewSystem_(
          content_.cards(),
          validator_,
          effectResolver_,
          damageSystem_,
          blockSystem_
      ),
      playerTurnSystem_(drawSystem_),
      enemyTurnSystem_(enemyMoveSelector_, effectSystem_),
      turnSystem_(
          content_.enemies(),
          playerTurnSystem_,
          enemyTurnSystem_,
          enemyMoveSelector_,
          5
      ),
      cardViewModelBuilder_(
          content_.cards(),
          localization_,
          previewSystem_
      ),
      combatViewModelBuilder_(
          localization_,
          cardViewModelBuilder_
      ) {
    initializeCombat();
}

void CombatScene::update(const float deltaSeconds) {
    const Vector2 mousePosition = GetMousePosition();

    if (combatFinished_) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (state_.phase == CombatPhase::Won) {
                onCombatWon_();
            } else {
                onCombatLost_();
            }
        }
        return;
    }

    if (draggedCardId_.has_value()) {
        view_.setDraggedCard(draggedCardId_, mousePosition);
    } else {
        view_.setDraggedCard(std::nullopt, mousePosition);
    }

    view_.setSelectedCard(selectedCardId_);
    view_.update(deltaSeconds, mousePosition);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        handleMousePressed(mousePosition);
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        handleMouseReleased(mousePosition);
    }

    const std::optional<EntityId> previewTarget = selectedCardId_.has_value()
        ? (view_.hoveredEnemyId().has_value()
            ? view_.hoveredEnemyId()
            : view_.hoveredPlayerId())
        : std::nullopt;

    if (previewTarget != lastPreviewTarget_) {
        lastPreviewTarget_ = previewTarget;
        viewModelDirty_ = true;
    }

    if (viewModelDirty_) {
        rebuildViewModel(previewTarget);
        view_.setSelectedCard(selectedCardId_);
        view_.setDraggedCard(draggedCardId_, mousePosition);
        viewModelDirty_ = false;
    }

    finishCombatIfNeeded();
}

void CombatScene::render() const {
    view_.render(uiFont_.available() ? &uiFont_.font() : nullptr);

    if (combatFinished_) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 130});
        const std::string message = state_.phase == CombatPhase::Won
            ? "Победа! Нажми Enter/Space/Click"
            : "Поражение. Нажми Enter/Space/Click";
        BasicUi::drawCenteredText(uiFont_, message, Rectangle{0.f, 0.f, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())}, 36.f, Color{245, 245, 250, 255});
    }
}

void CombatScene::initializeCombat() {
    if (!content_.cards().contains(CardId("strike")) ||
        !content_.cards().contains(CardId("defend")) ||
        !content_.cards().contains(CardId("poisoned_guard")) ||
        !content_.enemies().contains(EnemyId("training_dummy"))) {
        throw std::runtime_error("CombatScene requires strike, defend, poisoned_guard and training_dummy content");
    }

    state_ = CombatState{};
    entityIds_.reset();
    cardFactory_.reset();
    selectedCardId_.reset();
    draggedCardId_.reset();
    lastPreviewTarget_.reset();
    combatFinished_ = false;

    state_.resources.setMaxEnergy(3);

    playerId_ = entityIds_.create();
    state_.players.push_back(makeDebugPlayer(playerId_));

    const EntityId enemyId = entityIds_.create();
    const EnemyDefinition& enemyDefinition = content_.enemies().get(EnemyId("training_dummy"));
    state_.enemies.push_back(makeEnemyEntity(enemyDefinition, enemyId));
    state_.entity(enemyId).statuses.add("vulnerable", 2);

    if (runState_.deckCardIds.empty()) {
        throw std::runtime_error("Cannot initialize combat: run deck is empty");
    }

    for (const CardId& cardId : runState_.deckCardIds) {
        if (!content_.cards().contains(cardId)) {
            throw std::runtime_error("Run deck contains unknown card id: " + cardId.value);
        }

        state_.deck.drawPile.addTop(cardFactory_.create(cardId));
    }

    state_.log.add("Combat started");
    turnSystem_.startCombat(state_, random_);

    viewModelDirty_ = true;
    rebuildViewModel(std::nullopt);
    viewModelDirty_ = false;
}

void CombatScene::rebuildViewModel(const std::optional<EntityId> previewTarget) {
    view_.setModel(
        combatViewModelBuilder_.build(
            state_,
            playerId_,
            previewTarget
        )
    );
}

void CombatScene::handleMousePressed(const Vector2 mousePosition) {
    if (view_.endTurnButtonContains(mousePosition)) {
        endPlayerTurn();
        return;
    }

    if (view_.hoveredCardId().has_value()) {
        selectedCardId_ = *view_.hoveredCardId();
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

    selectedCardId_.reset();
    draggedCardId_.reset();
    viewModelDirty_ = true;
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
    selectedCardId_.reset();
    lastPreviewTarget_.reset();
    viewModelDirty_ = true;
}

void CombatScene::playSelectedCardOn(const EntityId target) {
    if (!selectedCardId_.has_value()) {
        return;
    }

    const PlayCardResult result = cardPlaySystem_.playCard(
        state_,
        PlayCardRequest{*selectedCardId_, playerId_, target},
        random_
    );

    if (!result.played) {
        state_.log.add("Cannot play card: " + result.reason);
    }

    selectedCardId_.reset();
    draggedCardId_.reset();
    lastPreviewTarget_.reset();

    if (state_.aliveEnemyIds().empty()) {
        state_.phase = CombatPhase::Won;
        state_.enemyIntents.clear();
        state_.log.add("Combat won");
    }

    viewModelDirty_ = true;
}

void CombatScene::endPlayerTurn() {
    selectedCardId_.reset();
    draggedCardId_.reset();
    lastPreviewTarget_.reset();
    turnSystem_.endPlayerTurn(state_, random_);
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
    const CardDefinition& definition = content_.cards().get(instance.definitionId);

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
    const CardDefinition& definition = content_.cards().get(instance.definitionId);

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

void CombatScene::finishCombatIfNeeded() {
    if (combatFinished_) {
        return;
    }

    if (state_.phase == CombatPhase::Won || state_.phase == CombatPhase::Lost) {
        combatFinished_ = true;
        viewModelDirty_ = true;
    }
}
