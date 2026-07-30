#include "CombatScene.hpp"
#include "ui/VirtualViewport.hpp"

#include "active_items/ActiveItemId.hpp"
#include "active_items/ActiveItemSystem.hpp"
#include "actors/PlayerActorDefinition.hpp"
#include "actors/PlayerActorId.hpp"
#include "cards/CardDefinition.hpp"
#include "cards/CardKeyword.hpp"
#include "cards/CardDescriptionFormatter.hpp"
#include "cards/CardUpgrade.hpp"
#include "combat/CombatPhase.hpp"
#include "consumables/ConsumableDefinition.hpp"
#include "effects/EffectTarget.hpp"
#include "enemies/EnemyInstance.hpp"
#include "relics/RelicDefinition.hpp"
#include "run/RunMapNode.hpp"
#include "run/SadistMasochistRules.hpp"
#include "run/StressRules.hpp"
#include "statuses/StatusDefinition.hpp"
#include "ui/BasicUi.hpp"
#include "ui/CardViewModelFactory.hpp"
#include "ui/CardVisualInstance.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
CombatEntity makePlayerFromActor(
    const PlayerActorDefinition& actor,
    const EntityId id,
    const RunActorState* runActorState
) {
    CombatEntity player;
    player.id = id;
    player.type = EntityType::Player;
    player.definitionId = actor.id.value;
    player.nameTextId = actor.nameTextId;

    if (runActorState != nullptr) {
        const int maximum = std::max(1, runActorState->maxHp);
        const int current = std::clamp(runActorState->currentHp, 0, maximum);
        player.health = Health(current, maximum);
        player.maxStress = std::max(StressRules::MaximumStress, runActorState->maxStress);
        player.stress = std::clamp(runActorState->stress, 0, player.maxStress);
        player.resolveCheckTriggered = runActorState->resolveCheckTriggered;
        player.traitIds = runActorState->traitIds;
        StressRules::normalize(player);
    } else {
        player.health = Health(actor.maxHp);
        player.maxStress = StressRules::MaximumStress;
        player.stress = 0;
        player.resolveCheckTriggered = false;
        player.traitIds = actor.startingTraitIds;
    }

    player.block = 0;
    return player;
}

const RunActorState* findRunActorState(const RunState& runState, const std::string& actorId) {
    for (const RunActorState& actorState : runState.actorStates) {
        if (actorState.definitionId == actorId) {
            return &actorState;
        }
    }

    return nullptr;
}

Vector2 blockedMousePosition() {
    return Vector2{-100000.f, -100000.f};
}


RunMapNodeType currentNodeType(const RunState& runState) {
    if (runState.map.currentNodeId < 0) {
        return RunMapNodeType::Combat;
    }

    for (const RunMapNode& node : runState.map.nodes) {
        if (node.id == runState.map.currentNodeId) {
            return node.type;
        }
    }

    return RunMapNodeType::Combat;
}

int currentNodeLayerIndex(const RunState& runState) {
    if (runState.map.currentNodeId < 0) {
        return 0;
    }

    const RunMapNode* current = nullptr;
    for (const RunMapNode& node : runState.map.nodes) {
        if (node.id == runState.map.currentNodeId) {
            current = &node;
            break;
        }
    }

    if (current == nullptr) {
        return 0;
    }

    std::vector<float> layerXs;
    layerXs.reserve(runState.map.nodes.size());
    for (const RunMapNode& node : runState.map.nodes) {
        const auto existing = std::find_if(
            layerXs.begin(),
            layerXs.end(),
            [&](const float x) {
                return std::abs(x - node.position.x) < 0.5f;
            }
        );

        if (existing == layerXs.end()) {
            layerXs.push_back(node.position.x);
        }
    }

    std::sort(layerXs.begin(), layerXs.end());
    for (std::size_t index = 0; index < layerXs.size(); ++index) {
        if (std::abs(layerXs[index] - current->position.x) < 0.5f) {
            return static_cast<int>(index);
        }
    }

    return 0;
}

std::vector<std::string> enemyIdsForNode(
    const RunState& runState,
    const EncounterDatabase& encounters,
    Random& random
) {
    const EncounterDefinition& encounter = encounters.choose(
        currentNodeType(runState),
        random,
        currentNodeLayerIndex(runState)
    );
    return encounter.enemyIds;
}

void addEnemyToCombat(
    CombatState& state,
    EntityIdGenerator& entityIds,
    const EnemyDatabase& enemies,
    const std::string& enemyId,
    const float hpMultiplier
) {
    if (!enemies.contains(EnemyId(enemyId))) {
        throw std::runtime_error("Unknown enemy in encounter: " + enemyId); // NOL10N: developer content validation diagnostic
    }

    const EnemyDefinition& definition = enemies.get(EnemyId(enemyId));
    CombatEntity enemy = makeEnemyEntity(definition, entityIds.create());

    if (hpMultiplier != 1.f) {
        const int scaledMaximum = std::max(
            1,
            static_cast<int>(static_cast<float>(enemy.health.maximum()) * hpMultiplier + 0.5f)
        );
        enemy.health.setMaximum(scaledMaximum);
        enemy.health.setCurrent(scaledMaximum);
    }

    state.enemies.push_back(std::move(enemy));
}

Vector2 rectangleCenter(const Rectangle bounds) {
    return Vector2{bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f};
}

Vector2 lerpVector(const Vector2 from, const Vector2 to, const float t) {
    const float clamped = std::clamp(t, 0.f, 1.f);
    return Vector2{
        from.x + (to.x - from.x) * clamped,
        from.y + (to.y - from.y) * clamped
    };
}

Vector2 quadraticBezierVector(const Vector2 start, const Vector2 control, const Vector2 end, const float t) {
    const float clamped = std::clamp(t, 0.f, 1.f);
    const Vector2 first = lerpVector(start, control, clamped);
    const Vector2 second = lerpVector(control, end, clamped);
    return lerpVector(first, second, clamped);
}

float smoothStep(const float t) {
    const float clamped = std::clamp(t, 0.f, 1.f);
    return clamped * clamped * (3.f - 2.f * clamped);
}

Color colorWithAlpha(const Color color, const float opacity) {
    const float clamped = std::clamp(opacity, 0.f, 1.f);
    return Color{
        color.r,
        color.g,
        color.b,
        static_cast<unsigned char>(static_cast<float>(color.a) * clamped)
    };
}

}

CombatScene::CombatScene(
    const ContentRegistry& content,
    const LocalizationManager& localization,
    Random& random,
    const UiFont& uiFont,
    RunState& runState,
    std::function<void(std::string)> onActiveItemUsed,
    std::function<void(const CombatResult&)> onCombatWon,
    std::function<void(const CombatResult&)> onCombatLost
)
    : content_(content),
      localization_(localization),
      random_(random),
      uiFont_(uiFont),
      runState_(runState),
      onActiveItemUsed_(std::move(onActiveItemUsed)),
      onCombatWon_(std::move(onCombatWon)),
      onCombatLost_(std::move(onCombatLost)),
      modifierSystem_(localization_, content_.statuses()),
      relicSystem_(content_.relics()),
      damageSystem_(modifierSystem_, &eventBus_),
      blockSystem_(modifierSystem_, &eventBus_),
      statusSystem_(content_.statuses(), &eventBus_),
      consumableSystem_(content_.consumables()),
      droneSystem_(
          content_.drones(),
          effectResolver_,
          targeting_,
          damageSystem_,
          blockSystem_,
          energySystem_,
          drawSystem_,
          statusSystem_,
          &eventBus_
      ),
      effectSystem_(
          effectResolver_,
          targeting_,
          damageSystem_,
          blockSystem_,
          energySystem_,
          drawSystem_,
          statusSystem_,
          droneSystem_,
          &eventBus_
      ),
      bossPhaseSystem_(content_.enemies(), effectSystem_),
      cardPlaySystem_(
          content_.cards(),
          validator_,
          energySystem_,
          effectSystem_,
          &eventBus_
      ),
      previewSystem_(
          content_.cards(),
          validator_,
          effectResolver_,
          damageSystem_,
          blockSystem_
      ),
      enemyMoveSelector_(modifierSystem_),
      playerTurnSystem_(drawSystem_, content_.cards(), &eventBus_, &effectSystem_),
      enemyTurnSystem_(enemyMoveSelector_, effectSystem_),
      turnSystem_(
          content_.enemies(),
          playerTurnSystem_,
          enemyTurnSystem_,
          enemyMoveSelector_,
          bossPhaseSystem_,
          statusSystem_,
          droneSystem_,
          combatController_,
          5,
          &eventBus_
      ),
      cardViewModelBuilder_(
          content_.cards(),
          localization_,
          previewSystem_
      ),
      combatViewModelBuilder_(
          localization_,
          content_.statuses(),
          content_.drones(),
          content_.cards(),
          content_.enemies(),
          cardViewModelBuilder_
      ),
      inspectModelBuilder_(content_, localization_) {
    relicSystem_.setRelics(runState_);
    eventBus_.subscribe([this](const GameEvent& event) {
        relicSystem_.handleEvent(state_, event, effectSystem_, random_);

        if (event.type == GameEventType::DamageTaken &&
            event.source.has_value() &&
            event.target.has_value() &&
            state_.hasEntity(*event.source) &&
            state_.hasEntity(*event.target) &&
            state_.isEnemy(*event.source) &&
            state_.isPlayer(*event.target)) {
            enqueueEnemyAttackAnimation(*event.source, *event.target);
        }

        switch (event.type) {
            case GameEventType::DamageDealt:
                enqueueDamageFeedback(event);
                break;
            case GameEventType::BlockGained:
                enqueueBlockFeedback(event);
                break;
            case GameEventType::Healed:
                enqueueHealFeedback(event);
                break;
            case GameEventType::StatusApplied:
                enqueueStatusFeedback(event);
                break;
            default:
                break;
        }

        if (isSadistMasochistParty()) {
            SadistMasochistRules::handleEvent(state_, event);
        }

        viewModelDirty_ = true;
    });

    initializeCombat();
}

void CombatScene::update(const float deltaSeconds) {
    const Vector2 mousePosition = GetMousePosition();
    updatePlayedCardAnimations(deltaSeconds);
    updateCombatFeedbackAnimations(deltaSeconds);
    updateEnemyAttackAnimations(deltaSeconds);
    updateGroupImpactAnimations(deltaSeconds);
    updateEnemyDeathAnimations(deltaSeconds);
    sanitizeTargetSelection();

    const bool wasResolvingEndTurn = pendingEndTurnResolution_;
    if (pendingEndTurnResolution_ && playedCardAnimations_.empty()) {
        resolvePendingEndTurn();
    }

    if (wasResolvingEndTurn) {
        selectedCardId_.reset();
        draggedCardId_.reset();
        keyboardTargetId_.reset();
        inspectedCardId_.reset();
        inspectedPileCardIndex_.reset();
        closeCombatItemInspect();
        relicInspectModal_.close();
        pendingConsumableIndex_.reset();
        targetingConsumableIndex_.reset();
        lastPreviewTarget_.reset();

        if (viewModelDirty_) {
            rebuildViewModel(std::nullopt);
            viewModelDirty_ = false;
        }

        view_.setSelectedCard(std::nullopt);
        view_.setDraggedCard(std::nullopt, blockedMousePosition());
        view_.update(deltaSeconds, blockedMousePosition());
        finishCombatIfNeeded();
        return;
    }

    if (combatFinished_) {
        selectedCardId_.reset();
        draggedCardId_.reset();
        keyboardTargetId_.reset();
        inspectedCardId_.reset();
        inspectedPileCardIndex_.reset();
        closeCombatItemInspect();
        relicInspectModal_.close();
        pendingConsumableIndex_.reset();
        targetingConsumableIndex_.reset();
        lastPreviewTarget_.reset();

        view_.setSelectedCard(std::nullopt);
        view_.setDraggedCard(std::nullopt, blockedMousePosition());
        view_.update(deltaSeconds, blockedMousePosition());

        if (state_.phase == CombatPhase::Won) {
            return;
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            onCombatLost_(finalResult_);
        }

        return;
    }

    if (state_.pendingForcedCardPlay.has_value() && playedCardAnimations_.empty()) {
        resolvePendingForcedCardPlay();
        if (viewModelDirty_) {
            rebuildViewModel(std::nullopt);
            viewModelDirty_ = false;
        }
        finishCombatIfNeeded();
        return;
    }

    if (!enemyAttackAnimations_.empty()) {
        selectedCardId_.reset();
        draggedCardId_.reset();
        keyboardTargetId_.reset();
        inspectedCardId_.reset();
        inspectedPileCardIndex_.reset();
        closeCombatItemInspect();
        relicInspectModal_.close();
        pendingConsumableIndex_.reset();
        targetingConsumableIndex_.reset();
        lastPreviewTarget_.reset();

        if (viewModelDirty_) {
            rebuildViewModel(std::nullopt);
            viewModelDirty_ = false;
        }

        view_.setSelectedCard(std::nullopt);
        view_.setDraggedCard(std::nullopt, blockedMousePosition());
        view_.update(deltaSeconds, blockedMousePosition());
        finishCombatIfNeeded();
        return;
    }

    if (pileOverlayMode_ != PileOverlayMode::None) {
        selectedCardId_.reset();
        draggedCardId_.reset();
        keyboardTargetId_.reset();
        inspectedCardId_.reset();
        closeCombatItemInspect();
        targetingConsumableIndex_.reset();
        lastPreviewTarget_.reset();

        view_.setSelectedCard(std::nullopt);
        view_.setDraggedCard(std::nullopt, blockedMousePosition());
        view_.update(deltaSeconds, blockedMousePosition());
        updatePileOverlay(mousePosition);
        return;
    }

    if (pendingConsumableIndex_.has_value()) {
        selectedCardId_.reset();
        draggedCardId_.reset();
        keyboardTargetId_.reset();
        inspectedCardId_.reset();
        inspectedPileCardIndex_.reset();
        closeCombatItemInspect();
        lastPreviewTarget_.reset();

        view_.setSelectedCard(std::nullopt);
        view_.setDraggedCard(std::nullopt, blockedMousePosition());
        view_.update(deltaSeconds, blockedMousePosition());
        updateConsumableConfirmationInput(mousePosition);
        return;
    }

    if (targetingConsumableIndex_.has_value()) {
        selectedCardId_.reset();
        draggedCardId_.reset();
        keyboardTargetId_.reset();
        inspectedCardId_.reset();
        inspectedPileCardIndex_.reset();
        closeCombatItemInspect();
        lastPreviewTarget_ = previewTargetForConsumable(*targetingConsumableIndex_);

        if (viewModelDirty_) {
            rebuildViewModel(lastPreviewTarget_);
            viewModelDirty_ = false;
        }
        view_.setSelectedCard(std::nullopt);
        view_.setDraggedCard(std::nullopt, mousePosition);
        view_.update(deltaSeconds, mousePosition);
        updateConsumableTargetingInput(mousePosition);
        return;
    }

    if (combatItemInspectOpen()) {
        selectedCardId_.reset();
        draggedCardId_.reset();
        keyboardTargetId_.reset();
        inspectedCardId_.reset();
        inspectedPileCardIndex_.reset();
        lastPreviewTarget_.reset();

        view_.setSelectedCard(std::nullopt);
        view_.setDraggedCard(std::nullopt, blockedMousePosition());
        view_.update(deltaSeconds, blockedMousePosition());
        updateCombatItemInspectInput(mousePosition);
        return;
    }

    if (relicInspectModal_.isOpen()) {
        relicInspectModal_.update(view_.model().relics.size());

        selectedCardId_.reset();
        draggedCardId_.reset();
        keyboardTargetId_.reset();
        inspectedCardId_.reset();
        inspectedPileCardIndex_.reset();
        lastPreviewTarget_.reset();

        view_.setSelectedCard(std::nullopt);
        view_.setDraggedCard(std::nullopt, blockedMousePosition());
        view_.update(deltaSeconds, blockedMousePosition());
        return;
    }

    if (draggedCardId_.has_value()) {
        view_.setDraggedCard(draggedCardId_, mousePosition);
    } else {
        view_.setDraggedCard(std::nullopt, mousePosition);
    }

    view_.setSelectedCard(selectedCardId_);
    view_.update(deltaSeconds, mousePosition);

    updateInspectInput(mousePosition);
    if (combatItemInspectOpen()) {
        selectedCardId_.reset();
        draggedCardId_.reset();
        keyboardTargetId_.reset();
        lastPreviewTarget_.reset();
        view_.setSelectedCard(std::nullopt);
        view_.setDraggedCard(std::nullopt, blockedMousePosition());
        view_.update(deltaSeconds, blockedMousePosition());
        return;
    }

    if (handleActiveItemInput()) {
        viewModelDirty_ = true;
        return;
    }

    handleKeyboardCombatInput();

    if (relicInspectModal_.isOpen()) {
        selectedCardId_.reset();
        draggedCardId_.reset();
        keyboardTargetId_.reset();
        inspectedCardId_.reset();
        inspectedPileCardIndex_.reset();
        closeCombatItemInspect();
        lastPreviewTarget_.reset();
        view_.setSelectedCard(std::nullopt);
        view_.setDraggedCard(std::nullopt, blockedMousePosition());
        view_.update(deltaSeconds, blockedMousePosition());
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (BasicUi::contains(drawPileButtonBounds(), mousePosition)) {
            openPileOverlay(PileOverlayMode::DrawPile);
            return;
        }
        if (BasicUi::contains(discardPileButtonBounds(), mousePosition)) {
            openPileOverlay(PileOverlayMode::DiscardPile);
            return;
        }
        if (BasicUi::contains(exhaustPileButtonBounds(), mousePosition)) {
            openPileOverlay(PileOverlayMode::ExhaustPile);
            return;
        }

        handleMousePressed(mousePosition);
        if (combatItemInspectOpen()) {
            selectedCardId_.reset();
            draggedCardId_.reset();
            keyboardTargetId_.reset();
            inspectedCardId_.reset();
            lastPreviewTarget_.reset();
            view_.setSelectedCard(std::nullopt);
            view_.setDraggedCard(std::nullopt, blockedMousePosition());
            view_.update(deltaSeconds, blockedMousePosition());
            return;
        }

        if (relicInspectModal_.isOpen()) {
            selectedCardId_.reset();
            draggedCardId_.reset();
            keyboardTargetId_.reset();
            inspectedCardId_.reset();
            lastPreviewTarget_.reset();
            view_.setSelectedCard(std::nullopt);
            view_.setDraggedCard(std::nullopt, blockedMousePosition());
            view_.update(deltaSeconds, blockedMousePosition());
            return;
        }
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        handleMouseReleased(mousePosition);
    }

    const std::optional<EntityId> previewTarget = previewTargetForSelectedCard();

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
    if (skipNextEnemyTurn_ && !combatFinished_) {
        BasicUi::drawCenteredTextFitted(
            uiFont_,
            localization_.get(TextId("active_item.hint.hourglass_armed")),
            Rectangle{static_cast<float>(VirtualViewport::width()) * 0.5f - 170.f, 92.f, 340.f, 28.f},
            18.f,
            13.f,
            Color{222, 194, 125, 255}
        );
    }
    const bool breakdownGuardArmed = std::any_of(
        state_.players.begin(),
        state_.players.end(),
        [this](const CombatEntity& player) { return player.isAlive() && state_.hasStressBreakdownGuard(player.id); }
    );
    if (breakdownGuardArmed && !combatFinished_) {
        BasicUi::drawCenteredTextFitted(
            uiFont_,
            localization_.get(TextId("active_item.hint.breakdown_guard_armed")),
            Rectangle{static_cast<float>(VirtualViewport::width()) * 0.5f - 190.f, 120.f, 380.f, 28.f},
            18.f,
            13.f,
            Color{164, 220, 204, 255}
        );
    }
    renderCombatFeedbackAnimations();
    renderGroupImpactAnimations();

    if (!combatFinished_) {
        renderPileButtons();
        renderPlayedCardAnimations();
        if (pileOverlayMode_ != PileOverlayMode::None) {
            renderPileOverlay();
            return;
        }

        if (pendingConsumableIndex_.has_value()) {
            renderConsumableConfirmationModal();
            return;
        }

        if (targetingConsumableIndex_.has_value()) {
            renderInspectOverlay();
            return;
        }

        if (combatItemInspectOpen()) {
            renderCombatItemInspectModal();
            return;
        }

        renderTargetingArrow();
        renderInspectOverlay();
        return;
    }

    if (state_.phase == CombatPhase::Won) {
        return;
    }

    renderDefeatModal();
}

void CombatScene::onLocalizationChanged() {
    viewModelDirty_ = true;
    lastPreviewTarget_.reset();
}

bool CombatScene::handleDebugCommand(const std::vector<std::string>& tokens, std::string& output) {
    if (tokens.empty()) {
        return false;
    }

    auto parseAmount = [](const std::vector<std::string>& values, const std::size_t index, const int fallback) {
        if (index >= values.size()) {
            return fallback;
        }

        try {
            return std::stoi(values[index]);
        } catch (...) {
            return fallback;
        }
    };

    auto firstTarget = [this](const std::string& side) -> std::optional<EntityId> {
        if (side == "enemy" || side == "enemies") {
            const std::vector<EntityId> enemies = state_.aliveEnemyIds();
            if (!enemies.empty()) {
                return enemies.front();
            }
            return std::nullopt;
        }

        const std::vector<EntityId> players = state_.alivePlayerIds();
        if (!players.empty()) {
            return players.front();
        }

        return std::nullopt;
    };

    auto localizedDebugTarget = [this](const std::string& side) {
        if (side == "enemy" || side == "enemies") {
            return localization_.get(TextId("debug.target.enemy"));
        }

        return localization_.get(TextId("debug.target.player"));
    };

    const std::string& command = tokens.front();

    if (command == "status" || (command == "apply" && tokens.size() >= 2 && tokens[1] == "status")) {
        const std::size_t idIndex = command == "status" ? 1u : 2u;
        if (tokens.size() <= idIndex) {
            output = localization_.get(TextId("debug.usage.status"));
            return true;
        }

        const std::string& statusId = tokens[idIndex];
        const int amount = parseAmount(tokens, idIndex + 1u, 1);
        std::string side = "player";
        if (tokens.size() > idIndex + 2u) {
            side = tokens[idIndex + 2u];
        }

        const std::optional<EntityId> target = firstTarget(side);
        if (!target.has_value()) {
            output = localization_.format(TextId("debug.no_alive_target"), {{"target", localizedDebugTarget(side)}});
            return true;
        }

        statusSystem_.applyStatus(state_, *target, statusId, amount);
        turnSystem_.refreshEnemyIntentValues(state_, random_);
        viewModelDirty_ = true;
        output = localization_.format(
            TextId("debug.status_applied"),
            {{"status", statusId}, {"amount", std::to_string(amount)}, {"target", localizedDebugTarget(side)}}
        );
        return true;
    }

    if (command == "heal" || command == "damage") {
        const int amount = parseAmount(tokens, 1u, 0);
        if (amount <= 0) {
            output = localization_.format(TextId("debug.usage.heal_damage"), {{"command", command}});
            return true;
        }

        std::string side = "player";
        if (tokens.size() > 2u) {
            side = tokens[2];
        }

        const std::optional<EntityId> target = firstTarget(side);
        if (!target.has_value()) {
            output = localization_.format(TextId("debug.no_alive_target"), {{"target", localizedDebugTarget(side)}});
            return true;
        }

        CombatEntity& entity = state_.entity(*target);
        if (command == "heal") {
            entity.health.heal(amount);
            output = localization_.format(
                TextId("debug.healed"),
                {{"target", localizedDebugTarget(side)}, {"amount", std::to_string(amount)}}
            );
        } else {
            entity.health.takeDamage(amount);
            output = localization_.format(
                TextId("debug.damaged"),
                {{"target", localizedDebugTarget(side)}, {"amount", std::to_string(amount)}}
            );
            combatController_.updateAfterAction(state_);
            finishCombatIfNeeded();
        }

        viewModelDirty_ = true;
        return true;
    }

    if (command == "stress") {
        const int amount = parseAmount(tokens, 1u, 0);
        if (tokens.size() < 2u || amount == 0) {
            output = localization_.get(TextId("debug.usage.stress"));
            return true;
        }

        std::string side = "player";
        if (tokens.size() > 2u) {
            side = tokens[2];
        }

        const std::optional<EntityId> target = firstTarget(side);
        if (!target.has_value()) {
            output = localization_.format(TextId("debug.no_alive_target"), {{"target", localizedDebugTarget(side)}});
            return true;
        }

        CombatEntity& entity = state_.entity(*target);
        const StressRules::StressAdjustmentResult result = StressRules::applyDelta(entity, amount, &random_);
        if (entity.type == EntityType::Player && result.collapsed) {
            entity.health.setCurrent(0);
            combatController_.updateAfterAction(state_);
            finishCombatIfNeeded();
        }
        viewModelDirty_ = true;
        output = localization_.format(
            TextId("debug.stress_adjusted"),
            {{"target", localizedDebugTarget(side)}, {"amount", std::to_string(result.applied)}}
        );
        return true;
    }

    if (command == "block") {
        const int amount = parseAmount(tokens, 1u, 0);
        if (amount <= 0) {
            output = localization_.get(TextId("debug.usage.block"));
            return true;
        }

        std::string side = "player";
        if (tokens.size() > 2u) {
            side = tokens[2];
        }

        const std::optional<EntityId> target = firstTarget(side);
        if (!target.has_value()) {
            output = localization_.format(TextId("debug.no_alive_target"), {{"target", localizedDebugTarget(side)}});
            return true;
        }

        state_.entity(*target).block += amount;
        viewModelDirty_ = true;
        output = localization_.format(
            TextId("debug.block_added"),
            {{"target", localizedDebugTarget(side)}, {"amount", std::to_string(amount)}}
        );
        return true;
    }

    if (command == "energy") {
        const int amount = parseAmount(tokens, 1u, 0);
        if (amount == 0) {
            output = localization_.get(TextId("debug.usage.energy"));
            return true;
        }

        if (amount > 0) {
            state_.resources.gainEnergy(amount);
        } else {
            const int spend = std::min(state_.resources.energy(), -amount);
            if (spend > 0) {
                state_.resources.spendEnergy(spend);
            }
        }

        viewModelDirty_ = true;
        output = localization_.format(TextId("debug.energy_adjusted"), {{"amount", std::to_string(amount)}});
        return true;
    }

    if (command == "win" && tokens.size() >= 2u && tokens[1] == "combat") {
        for (CombatEntity& enemy : state_.enemies) {
            enemy.health.setCurrent(0);
        }
        combatController_.updateAfterAction(state_);
        finishCombatIfNeeded();
        viewModelDirty_ = true;
        output = localization_.get(TextId("debug.combat_won"));
        return true;
    }

    if (command == "lose" && tokens.size() >= 2u && tokens[1] == "combat") {
        for (CombatEntity& player : state_.players) {
            player.health.setCurrent(0);
        }
        combatController_.updateAfterAction(state_);
        finishCombatIfNeeded();
        viewModelDirty_ = true;
        output = localization_.get(TextId("debug.combat_lost"));
        return true;
    }

    return false;
}

void CombatScene::initializeCombat() {
    state_ = CombatState{};
    entityIds_.reset();
    cardFactory_.reset();
    selectedCardId_.reset();
    draggedCardId_.reset();
    keyboardTargetId_.reset();
    inspectedCardId_.reset();
    pendingConsumableIndex_.reset();
    playedCardAnimations_.clear();
    enemyAttackAnimations_.clear();
    enemyDeathAnimations_.clear();
    encounteredEnemyIds_.clear();
    pileOverlayMode_ = PileOverlayMode::None;
    pileOverlayScrollOffset_ = 0.f;
    relicInspectModal_.close();
    lastPreviewTarget_.reset();
    combatFinished_ = false;
    finalResult_ = CombatResult{};
    reward_.reset();
    rewardSelection_ = RewardSelection{};
    activeRewardOptionIndex_.reset();
    selectedRewardCardIndex_.reset();
    rewardCardChoiceOpen_ = false;
    rewardAccepted_ = false;

    state_.resources.clearActorEnergy();
    state_.enemyHpMultiplier = runState_.enemyHpMultiplier;
    state_.enemyDamageMultiplier = runState_.enemyDamageMultiplier;
    // Sadist/Masochist has one shared hand and two separate energy pools.
    // The same hand is played in a fixed subturn order: Sadist, then
    // Masochist, then the enemies.
    state_.useSequentialPlayerTurns = isSadistMasochistParty();
    state_.activePlayerIndex = 0;

    if (runState_.actorDefinitionIds.empty()) {
        throw std::runtime_error("Cannot initialize combat: run has no player actors");
    }

    for (const std::string& actorId : runState_.actorDefinitionIds) {
        const PlayerActorDefinition& actor = content_.actors().get(PlayerActorId(actorId));
        const EntityId entityId = entityIds_.create();
        state_.players.push_back(makePlayerFromActor(actor, entityId, findRunActorState(runState_, actorId)));
        state_.resources.setMaxEnergy(entityId, actor.startingEnergy);
    }

    playerId_ = state_.players.front().id;
    combatConsumableIds_ = runState_.consumableIds;

    encounteredEnemyIds_ = enemyIdsForNode(
        runState_,
        content_.encountersForFloor(runState_.currentFloorId),
        random_
    );
    for (const std::string& enemyId : encounteredEnemyIds_) {
        addEnemyToCombat(
            state_,
            entityIds_,
            content_.enemies(),
            enemyId,
            runState_.enemyHpMultiplier
        );
    }

    if (state_.enemies.empty()) {
        throw std::runtime_error("Cannot initialize combat: encounter has no enemies");
    }

    if (runState_.deckCardIds.empty()) {
        throw std::runtime_error("Cannot initialize combat: run deck is empty");
    }

    for (std::size_t index = 0; index < runState_.deckCardIds.size(); ++index) {
        const CardId& cardId = runState_.deckCardIds[index];
        if (!content_.cards().contains(cardId)) {
            throw std::runtime_error("Run deck contains unknown card id: " + cardId.value); // NOL10N: developer save/content validation diagnostic
        }

        const bool upgraded = std::find(
            runState_.upgradedDeckIndices.begin(),
            runState_.upgradedDeckIndices.end(),
            static_cast<int>(index)
        ) != runState_.upgradedDeckIndices.end();
        state_.deck.drawPile.addTop(cardFactory_.create(cardId, upgraded));
    }

    state_.log.add(CombatLogEntryType::CombatStarted);
    relicSystem_.startCombat();
    turnSystem_.startCombat(state_, random_);

    viewModelDirty_ = true;
    rebuildViewModel(std::nullopt);
    viewModelDirty_ = false;
}

void CombatScene::rebuildViewModel(const std::optional<EntityId> previewTarget) {
    CombatViewModel model = combatViewModelBuilder_.build(
        state_,
        primaryPlayerId(),
        previewTarget,
        [this](const CardInstance& card) { return sourceForCard(card); }
    );

    if (!visuallyDiscardingCardIds_.empty()) {
        model.handCards.erase(
            std::remove_if(
                model.handCards.begin(),
                model.handCards.end(),
                [this](const CardViewModel& card) {
                    return isVisuallyDiscardingCard(card.instanceId);
                }
            ),
            model.handCards.end()
        );
    }

    if (isSadistMasochistParty()) {
        model.turnOrderLabel = localization_.get(TextId("ui.turn_order.sadist_masochist_sequence"));
        if (const CombatEntity* active = state_.activePlayer()) {
            model.activeActorLabel = localization_.format(TextId("ui.active_subturn.detail"), {
                {"actor", localizedOrFallback(active->nameTextId, active->definitionId)},
                {"current", std::to_string(state_.resources.energyFor(active->id))},
                {"max", std::to_string(state_.resources.maxEnergyFor(active->id))}
            });
        }
    }

    if (selectedCardId_.has_value() && state_.hand.contains(*selectedCardId_)) {
        bool selectedCardPlayable = false;
        for (const CardViewModel& card : model.handCards) {
            if (card.instanceId == *selectedCardId_) {
                selectedCardPlayable = card.playable;
                break;
            }
        }

        if (selectedCardPlayable) {
            const std::vector<EntityId> candidates = targetCandidatesForCard(*selectedCardId_);
            const auto isCandidate = [&candidates](const EntityId id) {
                return std::find(candidates.begin(), candidates.end(), id) != candidates.end();
            };

            bool hasEnemyCandidate = false;
            bool hasPlayerCandidate = false;
            for (EnemyViewModel& enemy : model.enemies) {
                enemy.targetable = isCandidate(enemy.entityId);
                enemy.previewTarget = previewTarget.has_value() && enemy.entityId == *previewTarget;
                hasEnemyCandidate = hasEnemyCandidate || enemy.targetable;
            }
            for (PlayerViewModel& player : model.players) {
                player.targetable = isCandidate(player.entityId);
                player.previewTarget = previewTarget.has_value() && player.entityId == *previewTarget;
                hasPlayerCandidate = hasPlayerCandidate || player.targetable;
            }

            if (hasEnemyCandidate && hasPlayerCandidate) {
                model.targetHintLabel = localizedOrFallback(TextId("ui.target_hint.any"), "Green outlines are valid targets. Yellow is the current preview target.");
            } else if (hasEnemyCandidate) {
                model.targetHintLabel = localizedOrFallback(TextId("ui.target_hint.enemy"), "Choose an enemy. Green outlines are valid targets, yellow is the preview target.");
            } else if (hasPlayerCandidate) {
                model.targetHintLabel = localizedOrFallback(TextId("ui.target_hint.player"), "Choose an ally. Green outlines are valid targets, yellow is the preview target.");
            } else {
                model.targetHintLabel = localizedOrFallback(TextId("ui.target_hint.none"), "This card has no selectable target.");
            }
        }
    }

    if (targetingConsumableIndex_.has_value()) {
        const std::vector<EntityId> candidates = targetCandidatesForConsumable(*targetingConsumableIndex_);
        const auto isCandidate = [&candidates](const EntityId id) {
            return std::find(candidates.begin(), candidates.end(), id) != candidates.end();
        };

        bool hasEnemyCandidate = false;
        bool hasPlayerCandidate = false;
        for (EnemyViewModel& enemy : model.enemies) {
            enemy.targetable = isCandidate(enemy.entityId);
            enemy.previewTarget = previewTarget.has_value() && enemy.entityId == *previewTarget;
            hasEnemyCandidate = hasEnemyCandidate || enemy.targetable;
        }
        for (PlayerViewModel& player : model.players) {
            player.targetable = isCandidate(player.entityId);
            player.previewTarget = previewTarget.has_value() && player.entityId == *previewTarget;
            hasPlayerCandidate = hasPlayerCandidate || player.targetable;
        }

        if (hasEnemyCandidate && hasPlayerCandidate) {
            model.targetHintLabel = localizedOrFallback(TextId("consumable.target_hint.any"), "Choose a target for the consumable.");
        } else if (hasEnemyCandidate) {
            model.targetHintLabel = localizedOrFallback(TextId("consumable.target_hint.enemy"), "Choose an enemy for the consumable.");
        } else if (hasPlayerCandidate) {
            model.targetHintLabel = localizedOrFallback(TextId("consumable.target_hint.player"), "Choose an ally for the consumable.");
        } else {
            model.targetHintLabel = localizedOrFallback(TextId("consumable.target_hint.none"), "This consumable has no valid target.");
        }
    }

    applyEnemyAttackVisuals(model);
    applyGroupImpactVisuals(model);
    applyCombatFeedbackVisuals(model);
    applyEnemyDeathVisuals(model);

    model.relics = buildRelicViewModels();
    attachActorRelicsToPlayers(model);
    model.droneSlotsLabel = localizedOrFallback(TextId("ui.drone_slots"), "Drone slots");
    model.emptyLabel = localizedOrFallback(TextId("ui.empty"), "Empty");
    model.droneSlots = buildDroneSlotViewModels();
    model.consumables.clear();
    model.consumables.reserve(static_cast<std::size_t>(runState_.maxConsumables));
    for (int i = 0; i < runState_.maxConsumables; ++i) {
        ConsumableViewModel consumable;
        if (i < static_cast<int>(combatConsumableIds_.size())) {
            consumable.id = combatConsumableIds_[static_cast<std::size_t>(i)];
            if (content_.consumables().contains(ConsumableId(consumable.id))) {
                const ConsumableDefinition& definition = content_.consumables().get(ConsumableId(consumable.id));
                consumable.name = localization_.get(definition.nameTextId);
                consumable.description = localization_.get(definition.descriptionTextId);
                consumable.filled = true;
            }
        }
        model.consumables.push_back(std::move(consumable));
    }
    view_.setModel(model);
}

EntityId CombatScene::primaryPlayerId() const {
    if (!state_.alivePlayerIds().empty()) {
        return state_.alivePlayerIds().front();
    }

    if (!state_.players.empty()) {
        return state_.players.front().id;
    }

    return playerId_;
}

EntityId CombatScene::sourceForCard(const CardInstance& card) const {
    if (!content_.cards().contains(card.definitionId)) {
        return primaryPlayerId();
    }

    const CardDefinition& definition = content_.cards().get(card.definitionId);
    if (definition.ownerActorId.empty()) {
        if (const std::optional<EntityId> activePlayer = state_.activePlayerId()) {
            return *activePlayer;
        }

        return primaryPlayerId();
    }

    for (const CombatEntity& player : state_.players) {
        if (player.definitionId == definition.ownerActorId && player.isAlive()) {
            return player.id;
        }
    }

    return primaryPlayerId();
}

EntityId CombatScene::sourceForCard(const CardInstanceId cardInstanceId) const {
    if (!state_.hand.contains(cardInstanceId)) {
        return primaryPlayerId();
    }

    return sourceForCard(state_.hand.get(cardInstanceId));
}

bool CombatScene::isSadistMasochistParty() const {
    return SadistMasochistRules::appliesTo(runState_.archetypeMechanicId);
}

bool CombatScene::canSelectCardSourceActors() const {
    return false;
}

void CombatScene::selectActivePlayerActor(const EntityId actorId) {
    if (!canSelectCardSourceActors()) {
        return;
    }

    for (std::size_t i = 0; i < state_.players.size(); ++i) {
        if (state_.players[i].id != actorId || !state_.players[i].isAlive()) {
            continue;
        }

        if (state_.activePlayerIndex == i) {
            return;
        }

        state_.activePlayerIndex = i;
        keyboardTargetId_.reset();
        lastPreviewTarget_.reset();
        if (selectedCardId_.has_value()) {
            ensureKeyboardTargetForSelectedCard();
        }
        viewModelDirty_ = true;
        return;
    }
}

void CombatScene::cycleActivePlayerActor(const int offset) {
    if (!canSelectCardSourceActors() || offset == 0) {
        return;
    }

    const int count = static_cast<int>(state_.players.size());
    if (count <= 0) {
        return;
    }

    const int current = static_cast<int>(std::min(state_.activePlayerIndex, state_.players.size() - 1u));
    for (int step = 1; step <= count; ++step) {
        const int index = (current + offset * step + count * step) % count;
        if (state_.players[static_cast<std::size_t>(index)].isAlive()) {
            selectActivePlayerActor(state_.players[static_cast<std::size_t>(index)].id);
            return;
        }
    }
}

bool CombatScene::isReplicantParty() const {
    return runState_.archetypeMechanicId == "replicant_drones";
}

std::vector<RelicViewModel> CombatScene::buildRelicViewModels() const {
    std::vector<RelicViewModel> result;

    auto appendRelic = [this, &result](const std::string& relicId, const std::string& ownerActorDefinitionId) {
        const RelicId id(relicId);

        RelicViewModel model;
        model.id = relicId;
        model.ownerActorDefinitionId = ownerActorDefinitionId;

        if (!ownerActorDefinitionId.empty() && content_.actors().contains(PlayerActorId(ownerActorDefinitionId))) {
            model.ownerName = localization_.get(content_.actors().get(PlayerActorId(ownerActorDefinitionId)).nameTextId);
        } else {
            model.ownerName = ownerActorDefinitionId;
        }

        if (content_.relics().contains(id)) {
            const RelicDefinition& definition = content_.relics().get(id);
            model.name = localization_.get(definition.nameTextId);
            model.description = localization_.get(definition.descriptionTextId);
        } else {
            model.name = relicId;
            model.description = {};
        }

        result.push_back(std::move(model));
    };

    for (const RunActorState& actor : runState_.actorStates) {
        for (const std::string& relicId : actor.relicIds) {
            appendRelic(relicId, actor.definitionId);
        }
    }

    for (const std::string& relicId : runState_.relicIds) {
        const bool alreadyVisible = std::any_of(result.begin(), result.end(), [&relicId](const RelicViewModel& model) {
            return model.id == relicId;
        });
        if (!alreadyVisible) {
            appendRelic(relicId, {});
        }
    }

    return result;
}

void CombatScene::attachActorRelicsToPlayers(CombatViewModel& model) const {
    bool hasActorOwnedRelics = false;
    bool hasUnownedRelics = false;

    for (PlayerViewModel& player : model.players) {
        player.relics.clear();

        for (const RelicViewModel& relic : model.relics) {
            if (relic.ownerActorDefinitionId.empty()) {
                hasUnownedRelics = true;
                continue;
            }

            if (relic.ownerActorDefinitionId == player.definitionId) {
                player.relics.push_back(relic);
                hasActorOwnedRelics = true;
            }
        }
    }

    model.showTopRelics = model.players.size() <= 1 || hasUnownedRelics || !hasActorOwnedRelics;
}

std::vector<DroneSlotViewModel> CombatScene::buildDroneSlotViewModels() const {
    if (!isReplicantParty() && state_.droneSlots.empty()) {
        return {};
    }

    const std::size_t slotCount = std::max<std::size_t>(state_.maxDroneSlots, state_.droneSlots.size());
    std::vector<DroneSlotViewModel> result;
    result.reserve(slotCount);

    for (std::size_t i = 0; i < slotCount; ++i) {
        DroneSlotViewModel slot;

        if (i < state_.droneSlots.size()) {
            const DroneSlot& combatSlot = state_.droneSlots[i];
            slot.filled = true;
            slot.type = combatSlot.droneId;

            const DroneId droneId(slot.type);
            if (content_.drones().contains(droneId)) {
                const DroneDefinition& definition = content_.drones().get(droneId);
                slot.name = localizedOrFallback(definition.nameTextId, slot.type);
                slot.description = localizedOrFallback(definition.descriptionTextId, slot.type);
                slot.cardActivationAvailable = definition.activeAction.has_value();
            } else {
                slot.name = slot.type;
                slot.description = localizedOrFallback(TextId("drone.unknown.description"), slot.type);
                slot.cardActivationAvailable = false;
            }

            slot.cardActivationLabel = slot.cardActivationAvailable
                ? localizedOrFallback(TextId("ui.drone_spend_by_card"), "Spent by activation cards")
                : localizedOrFallback(TextId("ui.drone_no_active_action"), "No card activation");
        } else {
            slot.filled = false;
            slot.type = {};
            slot.name = localizedOrFallback(TextId("drone.empty"), "Empty");
            slot.description = localizedOrFallback(TextId("drone.empty.description"), "This drone slot is empty.");
            slot.cardActivationAvailable = false;
            slot.cardActivationLabel = localizedOrFallback(TextId("ui.drone_empty"), "Empty");
        }

        result.push_back(std::move(slot));
    }

    return result;
}

bool CombatScene::resolvePendingForcedCardPlay() {
    if (!state_.pendingForcedCardPlay.has_value()) {
        return false;
    }

    const ForcedCardPlay request = *state_.pendingForcedCardPlay;
    state_.pendingForcedCardPlay.reset();

    if (!state_.hand.contains(request.cardInstanceId) || !state_.hasEntity(request.source)) {
        return false;
    }

    std::optional<EntityId> target = request.target;
    if (!target.has_value() || !state_.hasEntity(*target) || !state_.entity(*target).isAlive()) {
        const std::vector<EntityId> enemies = state_.aliveEnemyIds();
        if (enemies.empty()) {
            return false;
        }
        target = enemies.front();
    }

    selectedCardId_ = request.cardInstanceId;
    playSelectedCardOn(*target);
    return true;
}

void CombatScene::playSelectedCardOn(const EntityId target) {
    pendingConsumableIndex_.reset();

    if (!selectedCardId_.has_value()) {
        return;
    }

    const CardInstanceId playedCardId = *selectedCardId_;
    const EntityId source = sourceForCard(playedCardId);
    const bool groupImpact = cardAffectsAllEnemies(playedCardId);
    const std::vector<EntityId> groupImpactTargets = groupImpact ? state_.aliveEnemyIds() : std::vector<EntityId>{};
    std::optional<PlayedCardAnimation> pendingAnimation;
    if (state_.hand.contains(playedCardId)) {
        CardViewModel model = cardViewModelBuilder_.build(state_, playedCardId, source, target);
        model.selected = false;
        model.playable = true;

        const std::optional<Vector2> currentCenter = view_.cardCenter(playedCardId);
        pendingAnimation = PlayedCardAnimation{
            std::move(model),
            currentCenter.value_or(Vector2{playedCardCenterPosition().x, static_cast<float>(VirtualViewport::height()) - 150.f}),
            CardFlightAnimationKind::PlayedToDiscard,
            !playedCardAnimations_.empty(),
            0.f
        };
    }

    const PlayCardResult result = cardPlaySystem_.playCard(
        state_,
        PlayCardRequest{playedCardId, source, target},
        random_
    );

    if (!result.played) {
        state_.log.add(CombatLogEntryType::CannotPlayCard, {{"reason", result.reason}});
    } else {
        if (pendingAnimation.has_value()) {
            playedCardAnimations_.push_back(std::move(*pendingAnimation));
        }
        if (groupImpactTargets.size() > 1) {
            enqueueGroupImpactAnimation(groupImpactTargets);
        }
    }

    selectedCardId_.reset();
    draggedCardId_.reset();
    keyboardTargetId_.reset();
    inspectedCardId_.reset();
    relicInspectModal_.close();
    lastPreviewTarget_.reset();

    if (result.played) {
        finalResult_ = combatController_.updateAfterAction(state_);
        if (finalResult_.outcome == CombatOutcome::Ongoing) {
            turnSystem_.refreshEnemyIntentValues(state_, random_);
        }
    }

    viewModelDirty_ = true;
}


void CombatScene::updatePlayedCardAnimations(const float deltaSeconds) {
    if (playedCardAnimations_.empty()) {
        return;
    }

    const float clampedDelta = std::max(0.f, deltaSeconds);

    if (playedCardAnimations_.front().kind == CardFlightAnimationKind::HandToDiscard) {
        for (PlayedCardAnimation& animation : playedCardAnimations_) {
            if (animation.kind != CardFlightAnimationKind::HandToDiscard) {
                break;
            }
            animation.elapsedSeconds += clampedDelta;
        }

        playedCardAnimations_.erase(
            std::remove_if(
                playedCardAnimations_.begin(),
                playedCardAnimations_.end(),
                [](const PlayedCardAnimation& animation) {
                    if (animation.kind != CardFlightAnimationKind::HandToDiscard) {
                        return false;
                    }

                    const float duration = animation.durationSeconds > 0.f ? animation.durationSeconds : 0.42f;
                    return animation.elapsedSeconds >= duration;
                }
            ),
            playedCardAnimations_.end()
        );
        return;
    }

    PlayedCardAnimation& active = playedCardAnimations_.front();
    active.elapsedSeconds += clampedDelta;

    constexpr float totalDuration = 0.688f;
    if (active.elapsedSeconds >= totalDuration) {
        playedCardAnimations_.pop_front();
        if (!playedCardAnimations_.empty()) {
            PlayedCardAnimation& next = playedCardAnimations_.front();
            next.waitedInQueue = next.kind == CardFlightAnimationKind::PlayedToDiscard;
            next.elapsedSeconds = 0.f;
        }
    }
}

Vector2 CombatScene::playedCardCenterPosition() const {
    return Vector2{
        static_cast<float>(VirtualViewport::width()) * 0.5f,
        static_cast<float>(VirtualViewport::height()) * 0.47f
    };
}

Vector2 CombatScene::discardPileCenterPosition() const {
    return rectangleCenter(discardPileButtonBounds());
}

Vector2 CombatScene::playedCardQueuePosition(const std::size_t queueIndex) const {
    const Vector2 center = playedCardCenterPosition();
    const float offset = static_cast<float>(queueIndex - 1u);
    return Vector2{
        center.x - 250.f - offset * 42.f,
        center.y + 18.f + offset * 20.f
    };
}

CardTransform CombatScene::playedCardAnimationTransform(const PlayedCardAnimation& animation) const {
    constexpr float flyToCenterDuration = 0.192f;
    constexpr float holdDuration = 0.112f;
    constexpr float flyToDiscardDuration = 0.384f;

    const Vector2 center = playedCardCenterPosition();
    const Vector2 source = animation.waitedInQueue ? playedCardQueuePosition(1u) : animation.sourcePosition;
    const Vector2 discard = discardPileCenterPosition();
    const float baseScale = CardVisualInstance::standardScale();

    CardTransform transform;
    transform.zIndex = 9000;
    transform.rotationDegrees = 0.f;

    if (animation.elapsedSeconds < flyToCenterDuration) {
        const float t = smoothStep(animation.elapsedSeconds / flyToCenterDuration);
        transform.position = lerpVector(source, center, t);
        const float scale = baseScale * (1.0f + 0.16f * t);
        transform.scale = Vector2{scale, scale};
        transform.rotationDegrees = (animation.waitedInQueue ? -4.f : -10.f) * (1.f - t);
        return transform;
    }

    const float afterCenter = animation.elapsedSeconds - flyToCenterDuration;
    if (afterCenter < holdDuration) {
        const float scale = baseScale * 1.16f;
        transform.position = center;
        transform.scale = Vector2{scale, scale};
        return transform;
    }

    const float t = smoothStep((afterCenter - holdDuration) / flyToDiscardDuration);
    transform.position = lerpVector(center, discard, t);
    const float scale = baseScale * (1.16f + (0.20f - 1.16f) * t);
    transform.scale = Vector2{scale, scale};
    transform.rotationDegrees = -12.f * t;
    return transform;
}

CardTransform CombatScene::handDiscardAnimationTransform(const PlayedCardAnimation& animation) const {
    const float duration = animation.durationSeconds > 0.f ? animation.durationSeconds : 0.42f;
    const float t = smoothStep(animation.elapsedSeconds / duration);
    const float baseScale = CardVisualInstance::standardScale();
    const Vector2 discard = discardPileCenterPosition();
    const Vector2 midpoint = lerpVector(animation.sourcePosition, discard, 0.5f);
    const float arcHeight = animation.discardArcHeight > 0.f ? animation.discardArcHeight : 118.f;
    const Vector2 control{
        midpoint.x + animation.discardArcBend,
        std::min(animation.sourcePosition.y, discard.y) - arcHeight
    };

    CardTransform transform;
    transform.position = quadraticBezierVector(animation.sourcePosition, control, discard, t);
    const float scale = baseScale * (1.0f + (0.20f - 1.0f) * t);
    transform.scale = Vector2{scale, scale};
    const float bendRotation = animation.discardArcBend < 0.f ? -1.f : 1.f;
    transform.rotationDegrees = (-8.f * bendRotation) + (22.f * bendRotation * t);
    transform.zIndex = 8950;
    return transform;
}

void CombatScene::renderPlayedCardAnimations() const {
    if (playedCardAnimations_.empty()) {
        return;
    }

    const Font* font = uiFont_.available() ? &uiFont_.font() : nullptr;

    if (playedCardAnimations_.front().kind == CardFlightAnimationKind::HandToDiscard) {
        for (std::size_t i = 0u; i < playedCardAnimations_.size(); ++i) {
            const PlayedCardAnimation& animation = playedCardAnimations_[i];
            if (animation.kind != CardFlightAnimationKind::HandToDiscard) {
                break;
            }

            CardTransform transform = handDiscardAnimationTransform(animation);
            transform.zIndex = 8950 + static_cast<int>(i);
            CardVisualInstance::renderStatic(animation.model, font, transform);
        }
        return;
    }

    const std::size_t visibleQueuedCards = std::min<std::size_t>(playedCardAnimations_.size(), 5u);

    for (std::size_t i = visibleQueuedCards; i-- > 1u;) {
        const PlayedCardAnimation& animation = playedCardAnimations_[i];
        const float queueDepth = static_cast<float>(i - 1u);
        const float scale = CardVisualInstance::standardScale() * std::max(0.58f, 0.78f - queueDepth * 0.06f);
        const CardTransform transform{
            playedCardQueuePosition(i),
            Vector2{scale, scale},
            -5.f,
            8500 - static_cast<int>(i)
        };
        CardVisualInstance::renderStaticWithOverlay(animation.model, font, transform, Color{0, 0, 0, 110});
    }

    const PlayedCardAnimation& active = playedCardAnimations_.front();
    const CardTransform transform = playedCardAnimationTransform(active);
    CardVisualInstance::renderStatic(active.model, font, transform);
}

bool CombatScene::cardRetainsOnTurnEnd(const CardInstance& card) const {
    if (!content_.cards().contains(card.definitionId)) {
        return false;
    }

    const CardDefinition definition = CardUpgrade::effectiveDefinition(content_.cards().get(card.definitionId), card.upgraded);
    return std::find(definition.keywords.begin(), definition.keywords.end(), CardKeyword::Retain) != definition.keywords.end();
}

void CombatScene::enqueueEndTurnDiscardAnimations() {
    std::vector<const CardInstance*> discardingCards;
    discardingCards.reserve(state_.hand.size());
    visuallyDiscardingCardIds_.clear();

    for (const CardInstance& card : state_.hand.cards()) {
        if (cardRetainsOnTurnEnd(card)) {
            continue;
        }

        visuallyDiscardingCardIds_.push_back(card.instanceId);
        discardingCards.push_back(&card);
    }

    const float centerIndex = discardingCards.empty()
        ? 0.f
        : (static_cast<float>(discardingCards.size()) - 1.f) * 0.5f;

    for (std::size_t index = 0u; index < discardingCards.size(); ++index) {
        const CardInstance& card = *discardingCards[index];
        const EntityId source = sourceForCard(card);
        CardViewModel model = cardViewModelBuilder_.build(state_, card.instanceId, source, source);
        model.selected = false;
        model.playable = true;
        model.unplayableReason.clear();

        const float relativeIndex = static_cast<float>(index) - centerIndex;
        const std::optional<Vector2> currentCenter = view_.cardCenter(card.instanceId);
        PlayedCardAnimation animation{
            std::move(model),
            currentCenter.value_or(Vector2{playedCardCenterPosition().x, static_cast<float>(VirtualViewport::height()) - 150.f}),
            CardFlightAnimationKind::HandToDiscard,
            false,
            0.f,
            0.42f,
            112.f + std::abs(relativeIndex) * 22.f,
            relativeIndex * 34.f
        };

        playedCardAnimations_.push_back(std::move(animation));
    }
}

bool CombatScene::handleActiveItemInput() {
    if (!IsKeyPressed(KEY_SPACE) || state_.phase != CombatPhase::PlayerTurn || runState_.activeItem.empty()) {
        return false;
    }

    const ActiveItemId itemId(runState_.activeItem.itemId);
    if (!content_.activeItems().contains(itemId)) {
        return false;
    }

    const ActiveItemDefinition& definition = content_.activeItems().get(itemId);
    const bool skipsEnemyTurn = ActiveItemSystem::hasEffect(definition, ActiveItemEffectType::SkipEnemyTurn);
    const bool stabilizesStress = ActiveItemSystem::hasEffect(definition, ActiveItemEffectType::StabilizeStress);
    if (!skipsEnemyTurn && !stabilizesStress) {
        return false;
    }

    if (!ActiveItemSystem::canUse(runState_.activeItem, definition, ActiveItemUseContext::Combat)) {
        if (onActiveItemUsed_) {
            onActiveItemUsed_(localization_.format(
                TextId("active_item.feedback.not_charged"),
                {{"charge", std::to_string(runState_.activeItem.charge)}, {"cost", std::to_string(definition.chargeCost)}}
            ));
        }
        return true;
    }

    if (skipsEnemyTurn) {
        if (skipNextEnemyTurn_) {
            if (onActiveItemUsed_) {
                onActiveItemUsed_(localization_.get(TextId("active_item.feedback.enemy_turn_already_skipped")));
            }
            return true;
        }

        if (!ActiveItemSystem::spendCharge(runState_, definition, ActiveItemUseContext::Combat)) {
            return true;
        }

        skipNextEnemyTurn_ = true;
        if (onActiveItemUsed_) {
            onActiveItemUsed_(localization_.format(
                TextId("active_item.feedback.enemy_turn_skipped"),
                {{"item", localization_.get(definition.nameTextId)}}
            ));
        }
        return true;
    }

    CombatEntity* target = nullptr;
    for (CombatEntity& player : state_.players) {
        if (!player.isAlive()) {
            continue;
        }
        if (target == nullptr || player.stress > target->stress) {
            target = &player;
        }
    }
    if (target == nullptr) {
        return true;
    }

    int stressReduction = 0;
    for (const ActiveItemEffectDefinition& effect : definition.effects) {
        if (effect.type == ActiveItemEffectType::StabilizeStress) {
            stressReduction += effect.amount;
        }
    }

    const bool alreadyGuarded = state_.hasStressBreakdownGuard(target->id);
    const int beforeStress = target->stress;
    if (beforeStress <= 0 && alreadyGuarded) {
        if (onActiveItemUsed_) {
            onActiveItemUsed_(localization_.get(TextId("active_item.feedback.stress_already_stable")));
        }
        return true;
    }

    target->stress = std::max(0, target->stress - stressReduction);
    state_.armStressBreakdownGuard(target->id);
    if (!ActiveItemSystem::spendCharge(runState_, definition, ActiveItemUseContext::Combat)) {
        return true;
    }

    if (onActiveItemUsed_) {
        onActiveItemUsed_(localization_.format(
            TextId("active_item.feedback.stress_stabilized"),
            {
                {"item", localization_.get(definition.nameTextId)},
                {"amount", std::to_string(beforeStress - target->stress)}
            }
        ));
    }
    viewModelDirty_ = true;
    return true;
}

bool CombatScene::endPlayerTurnWillDiscardHand() const {
    if (state_.phase != CombatPhase::PlayerTurn) {
        return false;
    }

    if (!state_.useSequentialPlayerTurns) {
        return true;
    }

    const std::size_t start = std::min(state_.activePlayerIndex + 1u, state_.players.size());
    for (std::size_t i = start; i < state_.players.size(); ++i) {
        if (state_.players[i].isAlive()) {
            return false;
        }
    }

    return true;
}

void CombatScene::resolvePendingEndTurn() {
    if (!pendingEndTurnResolution_) {
        return;
    }

    pendingEndTurnResolution_ = false;
    visuallyDiscardingCardIds_.clear();

    turnSystem_.endPlayerTurn(state_, random_, skipNextEnemyTurn_);
    skipNextEnemyTurn_ = false;
    finalResult_ = combatController_.updateAfterAction(state_);
    viewModelDirty_ = true;
}

bool CombatScene::isVisuallyDiscardingCard(const CardInstanceId cardInstanceId) const {
    return std::find(
        visuallyDiscardingCardIds_.begin(),
        visuallyDiscardingCardIds_.end(),
        cardInstanceId
    ) != visuallyDiscardingCardIds_.end();
}

void CombatScene::endPlayerTurn() {
    if (pendingEndTurnResolution_) {
        return;
    }

    pendingConsumableIndex_.reset();
    selectedCardId_.reset();
    draggedCardId_.reset();
    keyboardTargetId_.reset();
    inspectedCardId_.reset();
    relicInspectModal_.close();
    lastPreviewTarget_.reset();

    if (!endPlayerTurnWillDiscardHand()) {
        visuallyDiscardingCardIds_.clear();
        turnSystem_.endPlayerTurn(state_, random_, skipNextEnemyTurn_);
        finalResult_ = combatController_.updateAfterAction(state_);
        viewModelDirty_ = true;
        return;
    }

    pendingEndTurnResolution_ = true;
    enqueueEndTurnDiscardAnimations();
    viewModelDirty_ = true;

    if (playedCardAnimations_.empty()) {
        resolvePendingEndTurn();
    }
}


void CombatScene::enqueueEnemyAttackAnimation(const EntityId enemyId, const EntityId targetId) {
    enemyAttackAnimations_.push_back(EnemyAttackAnimation{enemyId, targetId, 0.f});
    viewModelDirty_ = true;
}

void CombatScene::updateEnemyAttackAnimations(const float deltaSeconds) {
    if (enemyAttackAnimations_.empty()) {
        return;
    }

    constexpr float duration = 0.42f;
    EnemyAttackAnimation& active = enemyAttackAnimations_.front();
    active.elapsedSeconds += std::max(0.f, deltaSeconds);

    if (active.elapsedSeconds >= duration) {
        enemyAttackAnimations_.pop_front();
        if (!enemyAttackAnimations_.empty()) {
            enemyAttackAnimations_.front().elapsedSeconds = 0.f;
        }
    }

    viewModelDirty_ = true;
}

void CombatScene::applyEnemyAttackVisuals(CombatViewModel& model) const {
    if (enemyAttackAnimations_.empty()) {
        return;
    }

    constexpr float duration = 0.42f;
    const EnemyAttackAnimation& animation = enemyAttackAnimations_.front();
    const float progress = std::clamp(animation.elapsedSeconds / duration, 0.f, 1.f);

    Vector2 direction{-1.f, 0.f};
    const std::optional<Rectangle> enemyBounds = view_.enemyBounds(animation.enemyId);
    const std::optional<Rectangle> playerBounds = view_.playerBounds(animation.targetId);
    if (enemyBounds.has_value() && playerBounds.has_value()) {
        const Vector2 enemyCenter = rectangleCenter(*enemyBounds);
        const Vector2 playerCenter = rectangleCenter(*playerBounds);
        const float dx = playerCenter.x - enemyCenter.x;
        const float dy = playerCenter.y - enemyCenter.y;
        const float length = std::sqrt(dx * dx + dy * dy);
        if (length > 0.001f) {
            direction = Vector2{dx / length, dy / length};
        }
    }

    const float forward = progress < 0.28f
        ? smoothStep(progress / 0.28f)
        : 1.f - smoothStep((progress - 0.28f) / 0.72f);
    const float enemyDistance = 44.f * forward;

    for (EnemyViewModel& enemy : model.enemies) {
        if (enemy.entityId != animation.enemyId) {
            continue;
        }

        enemy.renderOffset.x += direction.x * enemyDistance;
        enemy.renderOffset.y += direction.y * enemyDistance;
        break;
    }

    if (progress < 0.16f) {
        return;
    }

    const float impactProgress = std::clamp((progress - 0.16f) / 0.84f, 0.f, 1.f);
    const float shakePower = (1.f - impactProgress) * 12.f;
    const Vector2 shake{
        std::sin(animation.elapsedSeconds * 160.f) * shakePower,
        std::sin(animation.elapsedSeconds * 113.f) * shakePower * 0.55f
    };

    for (PlayerViewModel& player : model.players) {
        if (player.entityId != animation.targetId) {
            continue;
        }

        player.renderOffset.x += shake.x;
        player.renderOffset.y += shake.y;
        break;
    }
}

void CombatScene::enqueueGroupImpactAnimation(std::vector<EntityId> targetIds) {
    targetIds.erase(
        std::remove_if(
            targetIds.begin(),
            targetIds.end(),
            [this](const EntityId targetId) {
                return !state_.hasEntity(targetId) || !state_.isEnemy(targetId);
            }
        ),
        targetIds.end()
    );

    if (targetIds.size() < 2) {
        return;
    }

    groupImpactAnimations_.push_back(GroupImpactAnimation{std::move(targetIds), 0.f});
    viewModelDirty_ = true;
}

void CombatScene::updateGroupImpactAnimations(const float deltaSeconds) {
    const float safeDelta = std::max(0.f, deltaSeconds);
    for (GroupImpactAnimation& animation : groupImpactAnimations_) {
        animation.elapsedSeconds += safeDelta;
    }

    const std::size_t previousSize = groupImpactAnimations_.size();
    groupImpactAnimations_.erase(
        std::remove_if(
            groupImpactAnimations_.begin(),
            groupImpactAnimations_.end(),
            [](const GroupImpactAnimation& animation) {
                return animation.elapsedSeconds >= 0.56f;
            }
        ),
        groupImpactAnimations_.end()
    );

    if (!groupImpactAnimations_.empty() || groupImpactAnimations_.size() != previousSize) {
        viewModelDirty_ = true;
    }
}

void CombatScene::applyGroupImpactVisuals(CombatViewModel& model) const {
    constexpr float duration = 0.56f;
    constexpr float pi = 3.14159265358979323846f;

    for (const GroupImpactAnimation& animation : groupImpactAnimations_) {
        const float progress = std::clamp(animation.elapsedSeconds / duration, 0.f, 1.f);
        const float pulse = std::sin(progress * pi);
        const float centerIndex = static_cast<float>(animation.targetIds.size() - 1) * 0.5f;

        for (std::size_t targetIndex = 0; targetIndex < animation.targetIds.size(); ++targetIndex) {
            for (EnemyViewModel& enemy : model.enemies) {
                if (enemy.entityId != animation.targetIds[targetIndex]) {
                    continue;
                }

                const float direction = static_cast<float>(targetIndex) - centerIndex;
                enemy.renderOffset.x += direction * 7.f * pulse;
                enemy.renderOffset.y -= 10.f * pulse;
                break;
            }
        }
    }
}

void CombatScene::renderGroupImpactAnimations() const {
    constexpr float duration = 0.56f;
    constexpr float pi = 3.14159265358979323846f;

    for (const GroupImpactAnimation& animation : groupImpactAnimations_) {
        const float progress = std::clamp(animation.elapsedSeconds / duration, 0.f, 1.f);
        const float pulse = std::sin(progress * pi);
        const float opacity = (1.f - progress) * 0.82f;
        std::vector<Vector2> centers;
        centers.reserve(animation.targetIds.size());

        for (const EntityId targetId : animation.targetIds) {
            const std::optional<Rectangle> bounds = view_.enemyBounds(targetId);
            if (!bounds.has_value()) {
                continue;
            }

            const Rectangle ring{
                bounds->x - 10.f * pulse,
                bounds->y - 10.f * pulse,
                bounds->width + 20.f * pulse,
                bounds->height + 20.f * pulse
            };
            DrawRectangleRoundedLinesEx(
                ring,
                0.08f,
                8,
                3.f + 2.f * pulse,
                colorWithAlpha(Color{255, 116, 92, 255}, opacity)
            );
            centers.push_back(rectangleCenter(*bounds));
        }

        for (std::size_t i = 1; i < centers.size(); ++i) {
            DrawLineEx(
                centers[i - 1],
                centers[i],
                3.f + 3.f * pulse,
                colorWithAlpha(Color{255, 155, 92, 255}, opacity * 0.72f)
            );
        }
    }
}

bool CombatScene::cardAffectsAllEnemies(const CardInstanceId cardInstanceId) const {
    if (!state_.hand.contains(cardInstanceId)) {
        return false;
    }

    const CardInstance& instance = state_.hand.get(cardInstanceId);
    const CardDefinition definition = CardUpgrade::effectiveDefinition(
        content_.cards().get(instance.definitionId),
        instance.upgraded
    );
    return std::any_of(
        definition.effects.begin(),
        definition.effects.end(),
        [](const EffectDefinition& effect) {
            return effect.target == EffectTarget::AllEnemies;
        }
    );
}

bool CombatScene::consumableAffectsAllEnemies(const std::size_t index) const {
    if (index >= combatConsumableIds_.size()) {
        return false;
    }

    const ConsumableId consumableId(combatConsumableIds_[index]);
    if (!content_.consumables().contains(consumableId)) {
        return false;
    }

    const ConsumableDefinition& definition = content_.consumables().get(consumableId);
    return std::any_of(
        definition.effects.begin(),
        definition.effects.end(),
        [](const EffectDefinition& effect) {
            return effect.target == EffectTarget::AllEnemies;
        }
    );
}

void CombatScene::sanitizeTargetSelection() {
    const auto isAliveEntity = [this](const std::optional<EntityId> id) {
        return id.has_value() && state_.hasEntity(*id) && state_.entity(*id).isAlive();
    };

    bool changed = false;
    if (keyboardTargetId_.has_value() && !isAliveEntity(keyboardTargetId_)) {
        keyboardTargetId_.reset();
        changed = true;
    }
    if (lastPreviewTarget_.has_value() && !isAliveEntity(lastPreviewTarget_)) {
        lastPreviewTarget_.reset();
        changed = true;
    }

    if (selectedCardId_.has_value()) {
        const std::optional<EntityId> before = keyboardTargetId_;
        ensureKeyboardTargetForSelectedCard();
        changed = changed || before != keyboardTargetId_;
    }

    if (changed) {
        viewModelDirty_ = true;
    }
}

void CombatScene::updateCombatFeedbackAnimations(const float deltaSeconds) {
    const float safeDelta = std::max(0.f, deltaSeconds);
    feedbackStaggerCursorSeconds_ = std::max(0.f, feedbackStaggerCursorSeconds_ - safeDelta * 2.6f);

    bool active = false;
    for (CombatFloatingFeedback& feedback : floatingFeedbacks_) {
        if (feedback.delaySeconds > 0.f) {
            feedback.delaySeconds = std::max(0.f, feedback.delaySeconds - safeDelta);
            active = true;
            continue;
        }

        feedback.elapsedSeconds += safeDelta;
        if (feedback.elapsedSeconds < 0.92f) {
            active = true;
        }
    }

    for (CombatHitFeedback& feedback : hitFeedbacks_) {
        if (feedback.delaySeconds > 0.f) {
            feedback.delaySeconds = std::max(0.f, feedback.delaySeconds - safeDelta);
            active = true;
            continue;
        }

        feedback.elapsedSeconds += safeDelta;
        if (feedback.elapsedSeconds < 0.30f) {
            active = true;
        }
    }

    floatingFeedbacks_.erase(
        std::remove_if(
            floatingFeedbacks_.begin(),
            floatingFeedbacks_.end(),
            [](const CombatFloatingFeedback& feedback) {
                return feedback.delaySeconds <= 0.f && feedback.elapsedSeconds >= 0.92f;
            }
        ),
        floatingFeedbacks_.end()
    );

    hitFeedbacks_.erase(
        std::remove_if(
            hitFeedbacks_.begin(),
            hitFeedbacks_.end(),
            [](const CombatHitFeedback& feedback) {
                return feedback.delaySeconds <= 0.f && feedback.elapsedSeconds >= 0.30f;
            }
        ),
        hitFeedbacks_.end()
    );

    if (active) {
        viewModelDirty_ = true;
    }
}

void CombatScene::enqueueFloatingFeedback(
    const EntityId targetId,
    std::string text,
    const CombatFeedbackKind kind
) {
    if (!state_.hasEntity(targetId) || text.empty()) {
        return;
    }

    const float delay = std::min(feedbackStaggerCursorSeconds_, 0.36f);
    feedbackStaggerCursorSeconds_ = std::min(feedbackStaggerCursorSeconds_ + 0.075f, 0.44f);
    floatingFeedbacks_.push_back(CombatFloatingFeedback{targetId, std::move(text), kind, delay, 0.f});
}

void CombatScene::enqueueHitFeedback(const EntityId targetId, const CombatFeedbackKind kind) {
    if (!state_.hasEntity(targetId)) {
        return;
    }

    const float delay = std::min(feedbackStaggerCursorSeconds_, 0.36f);
    hitFeedbacks_.push_back(CombatHitFeedback{targetId, kind, delay, 0.f});
}

void CombatScene::enqueueDamageFeedback(const GameEvent& event) {
    if (!event.target.has_value() || !state_.hasEntity(*event.target)) {
        return;
    }

    if (event.blockedAmount > 0) {
        enqueueFloatingFeedback(
            *event.target,
            localization_.get(TextId("combat.feedback.block")) + " -" + std::to_string(event.blockedAmount),
            CombatFeedbackKind::Block
        );
    }

    if (event.amount > 0) {
        enqueueFloatingFeedback(*event.target, std::string("-") + std::to_string(event.amount), CombatFeedbackKind::Damage);
        enqueueHitFeedback(*event.target, CombatFeedbackKind::Damage);
        return;
    }

    if (event.blockedAmount > 0) {
        enqueueHitFeedback(*event.target, CombatFeedbackKind::Block);
    }
}

void CombatScene::enqueueBlockFeedback(const GameEvent& event) {
    if (!event.target.has_value() || event.amount <= 0) {
        return;
    }

    enqueueFloatingFeedback(
        *event.target,
        std::string("+") + std::to_string(event.amount) + " " + localization_.get(TextId("combat.feedback.block")),
        CombatFeedbackKind::Block
    );
    enqueueHitFeedback(*event.target, CombatFeedbackKind::Block);
}

void CombatScene::enqueueHealFeedback(const GameEvent& event) {
    if (!event.target.has_value() || event.amount <= 0) {
        return;
    }

    enqueueFloatingFeedback(
        *event.target,
        std::string("+") + std::to_string(event.amount) + " " + localization_.get(TextId("combat.feedback.hp")),
        CombatFeedbackKind::Heal
    );
}

void CombatScene::enqueueStatusFeedback(const GameEvent& event) {
    if (!event.target.has_value() || event.statusId.empty() || event.amount <= 0) {
        return;
    }

    std::string statusName = event.statusId;
    const StatusId statusId(event.statusId);
    if (content_.statuses().contains(statusId)) {
        statusName = localization_.get(content_.statuses().get(statusId).nameTextId);
    }

    enqueueFloatingFeedback(*event.target, std::string("+") + std::to_string(event.amount) + " " + statusName, CombatFeedbackKind::Status);
}

void CombatScene::applyCombatFeedbackVisuals(CombatViewModel& model) const {
    for (const CombatHitFeedback& feedback : hitFeedbacks_) {
        if (feedback.delaySeconds > 0.f || feedback.elapsedSeconds >= 0.30f) {
            continue;
        }

        const float progress = std::clamp(feedback.elapsedSeconds / 0.30f, 0.f, 1.f);
        const float power = (1.f - progress) * (feedback.kind == CombatFeedbackKind::Damage ? 8.f : 4.f);
        const Vector2 shake{
            std::sin(feedback.elapsedSeconds * 145.f) * power,
            std::sin(feedback.elapsedSeconds * 101.f) * power * 0.55f
        };

        for (EnemyViewModel& enemy : model.enemies) {
            if (enemy.entityId == feedback.targetId) {
                enemy.renderOffset.x += shake.x;
                enemy.renderOffset.y += shake.y;
                break;
            }
        }

        for (PlayerViewModel& player : model.players) {
            if (player.entityId == feedback.targetId) {
                player.renderOffset.x += shake.x;
                player.renderOffset.y += shake.y;
                break;
            }
        }
    }
}

Color CombatScene::feedbackColor(const CombatFeedbackKind kind, const float opacity) const {
    switch (kind) {
        case CombatFeedbackKind::Damage:
            return colorWithAlpha(Color{255, 92, 82, 255}, opacity);
        case CombatFeedbackKind::Block:
            return colorWithAlpha(Color{120, 198, 255, 255}, opacity);
        case CombatFeedbackKind::Heal:
            return colorWithAlpha(Color{108, 235, 158, 255}, opacity);
        case CombatFeedbackKind::Status:
            return colorWithAlpha(Color{238, 214, 106, 255}, opacity);
    }

    return colorWithAlpha(WHITE, opacity);
}

Rectangle CombatScene::feedbackTargetBounds(const EntityId targetId) const {
    if (state_.hasEntity(targetId) && state_.isEnemy(targetId)) {
        if (const std::optional<Rectangle> bounds = view_.enemyBounds(targetId)) {
            return *bounds;
        }
    }

    if (state_.hasEntity(targetId) && state_.isPlayer(targetId)) {
        if (const std::optional<Rectangle> bounds = view_.playerBounds(targetId)) {
            return *bounds;
        }
    }

    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - 80.f,
        static_cast<float>(VirtualViewport::height()) * 0.5f - 60.f,
        160.f,
        120.f
    };
}

Vector2 CombatScene::feedbackAnchor(const EntityId targetId) const {
    const Rectangle bounds = feedbackTargetBounds(targetId);
    return Vector2{bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.18f};
}

void CombatScene::renderCombatFeedbackAnimations() const {
    for (const CombatHitFeedback& feedback : hitFeedbacks_) {
        if (feedback.delaySeconds > 0.f || feedback.elapsedSeconds >= 0.30f) {
            continue;
        }

        const Rectangle bounds = feedbackTargetBounds(feedback.targetId);
        const float progress = std::clamp(feedback.elapsedSeconds / 0.30f, 0.f, 1.f);
        const float opacity = 0.38f * (1.f - progress);
        const Color color = feedbackColor(feedback.kind, 1.f);
        DrawRectangleRec(bounds, colorWithAlpha(color, opacity));
        DrawRectangleLinesEx(bounds, 4.f, feedbackColor(feedback.kind, 0.82f * (1.f - progress)));
    }

    if (!uiFont_.available()) {
        return;
    }

    const Font& font = uiFont_.font();
    for (const CombatFloatingFeedback& feedback : floatingFeedbacks_) {
        if (feedback.delaySeconds > 0.f || feedback.elapsedSeconds >= 0.92f) {
            continue;
        }

        const float progress = std::clamp(feedback.elapsedSeconds / 0.92f, 0.f, 1.f);
        const float fadeStart = 0.58f;
        const float opacity = progress < fadeStart
            ? 1.f
            : 1.f - smoothStep((progress - fadeStart) / (1.f - fadeStart));
        const Vector2 anchor = feedbackAnchor(feedback.targetId);
        const float yOffset = -28.f - 62.f * smoothStep(progress);
        const float xOffset = std::sin(feedback.elapsedSeconds * 7.4f) * 8.f;
        const float fontSize = feedback.kind == CombatFeedbackKind::Damage ? 30.f : 24.f;
        const Vector2 textSize = MeasureTextEx(font, feedback.text.c_str(), fontSize, 1.f);
        const Vector2 position{anchor.x - textSize.x * 0.5f + xOffset, anchor.y + yOffset};
        const Color textColor = feedbackColor(feedback.kind, opacity);
        DrawTextEx(font, feedback.text.c_str(), Vector2{position.x + 2.f, position.y + 2.f}, fontSize, 1.f, colorWithAlpha(BLACK, 0.58f * opacity));
        DrawTextEx(font, feedback.text.c_str(), position, fontSize, 1.f, textColor);
    }
}

void CombatScene::updateEnemyDeathAnimations(const float deltaSeconds) {
    if (enemyDeathAnimations_.empty()) {
        return;
    }

    bool active = false;
    for (EnemyDeathAnimation& animation : enemyDeathAnimations_) {
        if (animation.elapsedSeconds < 0.72f) {
            animation.elapsedSeconds = std::min(0.72f, animation.elapsedSeconds + std::max(0.f, deltaSeconds));
            active = true;
        }
    }

    if (active) {
        viewModelDirty_ = true;
    }
}

bool CombatScene::deathAnimationExists(const EntityId enemyId) const {
    return std::any_of(
        enemyDeathAnimations_.begin(),
        enemyDeathAnimations_.end(),
        [enemyId](const EnemyDeathAnimation& animation) {
            return animation.enemyId == enemyId;
        }
    );
}

void CombatScene::startDeathAnimationsForNewlyDeadEnemies() {
    for (const CombatEntity& enemy : state_.enemies) {
        if (enemy.isAlive() || deathAnimationExists(enemy.id)) {
            continue;
        }

        enemyDeathAnimations_.push_back(EnemyDeathAnimation{enemy.id, 0.f});
        viewModelDirty_ = true;
    }
}

bool CombatScene::enemyDeathAnimationsComplete() const {
    for (const CombatEntity& enemy : state_.enemies) {
        if (enemy.isAlive()) {
            continue;
        }

        const auto iterator = std::find_if(
            enemyDeathAnimations_.begin(),
            enemyDeathAnimations_.end(),
            [&enemy](const EnemyDeathAnimation& animation) {
                return animation.enemyId == enemy.id;
            }
        );

        if (iterator == enemyDeathAnimations_.end() || iterator->elapsedSeconds < 0.72f) {
            return false;
        }
    }

    return true;
}

void CombatScene::applyEnemyDeathVisuals(CombatViewModel& model) const {
    for (EnemyViewModel& enemy : model.enemies) {
        const auto iterator = std::find_if(
            enemyDeathAnimations_.begin(),
            enemyDeathAnimations_.end(),
            [&enemy](const EnemyDeathAnimation& animation) {
                return animation.enemyId == enemy.entityId;
            }
        );

        if (iterator == enemyDeathAnimations_.end()) {
            continue;
        }

        const float progress = std::clamp(iterator->elapsedSeconds / 0.72f, 0.f, 1.f);
        const float shakePower = (1.f - progress) * 9.f;
        enemy.opacity = std::max(0.f, 1.f - progress);
        enemy.renderOffset = Vector2{
            std::sin(iterator->elapsedSeconds * 84.f) * shakePower,
            std::sin(iterator->elapsedSeconds * 57.f) * shakePower * 0.45f
        };
        enemy.targetable = false;
        enemy.previewTarget = false;
        if (progress >= 1.f) {
            enemy.opacity = 0.f;
        }
    }
}

void CombatScene::finishCombatIfNeeded() {
    if (combatFinished_) {
        return;
    }

    finalResult_ = combatController_.buildResult(state_);
    for (const CombatEntity& enemy : state_.enemies) {
        if (std::find(encounteredEnemyIds_.begin(), encounteredEnemyIds_.end(), enemy.definitionId) == encounteredEnemyIds_.end()) {
            encounteredEnemyIds_.push_back(enemy.definitionId);
        }
    }
    finalResult_.encounteredEnemyIds = encounteredEnemyIds_;
    finalResult_.remainingConsumableIds = combatConsumableIds_;
    startDeathAnimationsForNewlyDeadEnemies();

    if (finalResult_.outcome == CombatOutcome::Victory) {
        selectedCardId_.reset();
        draggedCardId_.reset();
        keyboardTargetId_.reset();
        inspectedCardId_.reset();
        inspectedPileCardIndex_.reset();
        relicInspectModal_.close();
        pendingConsumableIndex_.reset();
        targetingConsumableIndex_.reset();
        lastPreviewTarget_.reset();
        viewModelDirty_ = true;

        if (!enemyDeathAnimationsComplete() || !playedCardAnimations_.empty()) {
            return;
        }

        combatFinished_ = true;
        if (onCombatWon_) {
            onCombatWon_(finalResult_);
        }
        return;
    }

    if (finalResult_.outcome == CombatOutcome::Defeat) {
        combatFinished_ = true;
        selectedCardId_.reset();
        draggedCardId_.reset();
        keyboardTargetId_.reset();
        inspectedCardId_.reset();
        inspectedPileCardIndex_.reset();
        relicInspectModal_.close();
        pendingConsumableIndex_.reset();
        lastPreviewTarget_.reset();
        viewModelDirty_ = true;
    }
}
