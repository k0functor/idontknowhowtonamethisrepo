#include "CombatScene.hpp"
#include "ui/VirtualViewport.hpp"

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

void drawModalBackdrop() {
    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 155});
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

CombatScene::CombatScene(
    const ContentRegistry& content,
    const LocalizationManager& localization,
    Random& random,
    const UiFont& uiFont,
    const RunState& runState,
    std::function<void(const CombatResult&)> onCombatWon,
    std::function<void(const CombatResult&)> onCombatLost
)
    : content_(content),
      localization_(localization),
      random_(random),
      uiFont_(uiFont),
      runState_(runState),
      onCombatWon_(std::move(onCombatWon)),
      onCombatLost_(std::move(onCombatLost)),
      modifierSystem_(localization_),
      relicSystem_(content_.relics(), localization_),
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
      playerTurnSystem_(drawSystem_, content_.cards()),
      enemyTurnSystem_(enemyMoveSelector_, effectSystem_),
      turnSystem_(
          content_.enemies(),
          playerTurnSystem_,
          enemyTurnSystem_,
          enemyMoveSelector_,
          statusSystem_,
          droneSystem_,
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
          cardViewModelBuilder_
      ),
      inspectModelBuilder_(content_, localization_) {
    relicSystem_.setRelics(runState_.relicIds);
    modifierSystem_.addProvider(relicSystem_);
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

        if (isSadistMasochistParty() &&
            event.type == GameEventType::DamageDealt &&
            event.source.has_value() &&
            event.target.has_value() &&
            event.amount > 0 &&
            state_.hasEntity(*event.source) &&
            state_.hasEntity(*event.target)) {
            CombatEntity& source = state_.entity(*event.source);
            CombatEntity& target = state_.entity(*event.target);

            if (source.definitionId == "sadist" && target.definitionId == "masochist") {
                source.statuses.add("strength", 1);
                state_.log.add(CombatLogEntryType::SadistHurtsMasochist);
            }
        }

        if (isSadistMasochistParty() &&
            event.type == GameEventType::DamageTaken &&
            event.target.has_value() &&
            event.amount > 0 &&
            state_.hasEntity(*event.target)) {
            CombatEntity& target = state_.entity(*event.target);
            if (target.definitionId == "masochist") {
                target.statuses.add("strength", 1);
                target.statuses.add("dexterity", 1);
                state_.log.add(CombatLogEntryType::MasochistPainBonus);
            }
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
    updateEnemyDeathAnimations(deltaSeconds);

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
    renderCombatFeedbackAnimations();

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
        turnSystem_.refreshEnemyIntentValues(state_);
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

    const std::vector<std::string> encounterEnemyIds = enemyIdsForNode(runState_, content_.encounters(), random_);
    for (const std::string& enemyId : encounterEnemyIds) {
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
        model.turnOrderLabel = localizedOrFallback(TextId("ui.turn_order.sadist_masochist"), "Turn order: Sadist -> Masochist -> Enemy");
        if (const CombatEntity* active = state_.activePlayer()) {
            model.activeActorLabel = localizedOrFallback(TextId("ui.active_actor"), "Acting") + ": " + localizedOrFallback(active->nameTextId, active->definitionId);
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
    applyCombatFeedbackVisuals(model);
    applyEnemyDeathVisuals(model);

    model.relics = buildRelicViewModels();
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
    return runState_.archetypeMechanicId == "sadist_masochist_party";
}

bool CombatScene::isDroneCyborgParty() const {
    return runState_.archetypeMechanicId == "replicant_drones";
}

std::vector<RelicViewModel> CombatScene::buildRelicViewModels() const {
    std::vector<RelicViewModel> result;
    result.reserve(runState_.relicIds.size());

    for (const std::string& relicId : runState_.relicIds) {
        const RelicId id(relicId);

        RelicViewModel model;
        model.id = relicId;

        if (content_.relics().contains(id)) {
            const RelicDefinition& definition = content_.relics().get(id);
            model.name = localization_.get(definition.nameTextId);
            model.description = localization_.get(definition.descriptionTextId);
        } else {
            model.name = relicId;
            model.description = {};
        }

        result.push_back(std::move(model));
    }

    return result;
}

std::vector<DroneSlotViewModel> CombatScene::buildDroneSlotViewModels() const {
    if (!isDroneCyborgParty() && state_.droneSlots.empty()) {
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

void CombatScene::updateInspectInput(const Vector2) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        inspectedCardId_.reset();
        closeCombatItemInspect();
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        if (view_.hoveredRelicIndex().has_value()) {
            openRelicInspect(*view_.hoveredRelicIndex());
            return;
        }

        if (view_.hoveredConsumableIndex().has_value()) {
            openConsumableInspect(*view_.hoveredConsumableIndex());
            return;
        }

        if (view_.hoveredCardId().has_value()) {
            inspectedCardId_ = *view_.hoveredCardId();
            closeCombatItemInspect();
            return;
        }
    }

    if (IsKeyPressed(KEY_I)) {
        if (view_.hoveredRelicIndex().has_value()) {
            openRelicInspect(*view_.hoveredRelicIndex());
            return;
        }

        if (view_.hoveredConsumableIndex().has_value()) {
            openConsumableInspect(*view_.hoveredConsumableIndex());
            return;
        }

        if (selectedCardId_.has_value()) {
            inspectedCardId_ = selectedCardId_;
            closeCombatItemInspect();
            return;
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (!view_.hoveredCardId().has_value()) {
            inspectedCardId_.reset();
        }
    }
}

void CombatScene::renderInspectOverlay() const {
    if (relicInspectModal_.isOpen()) {
        relicInspectModal_.render(uiFont_, localization_, view_.model().relics);
        return;
    }

    if (inspectedCardId_.has_value()) {
        const std::optional<CardViewModel> cardModel = inspectedCardViewModel();
        if (cardModel.has_value() && state_.hand.contains(*inspectedCardId_)) {
            const CardInstance& instance = state_.hand.get(*inspectedCardId_);
            const CardDefinition definition = CardUpgrade::effectiveDefinition(content_.cards().get(instance.definitionId), instance.upgraded);
            const InspectPanelModel panel = inspectModelBuilder_.buildCard(definition, *cardModel);

            const float width = std::min(540.f, static_cast<float>(VirtualViewport::width()) - 60.f);
            const Rectangle bounds{
                static_cast<float>(VirtualViewport::width()) - width - 24.f,
                94.f,
                width,
                std::min(660.f, static_cast<float>(VirtualViewport::height()) - 120.f)
            };
            inspectPanelView_.render(uiFont_, panel, bounds);
        }
        return;
    }

    if (view_.hoveredRelicIndex().has_value()) {
        const std::size_t index = *view_.hoveredRelicIndex();
        if (index < view_.model().relics.size()) {
            const RelicViewModel& relicModel = view_.model().relics[index];
            InspectPanelModel panel;
            if (content_.relics().contains(RelicId(relicModel.id))) {
                panel = inspectModelBuilder_.buildRelic(content_.relics().get(RelicId(relicModel.id)));
            } else {
                panel.header = relicModel.name;
                panel.subheader = relicModel.description;
            }

            const float screenWidth = static_cast<float>(VirtualViewport::width());
            const float screenHeight = static_cast<float>(VirtualViewport::height());
            constexpr float screenMargin = 18.f;
            const float width = std::min(460.f, screenWidth - screenMargin * 2.f);
            const Rectangle bounds{
                screenMargin,
                78.f,
                width,
                std::min(500.f, screenHeight - 120.f)
            };
            inspectPanelView_.render(uiFont_, panel, bounds);
            return;
        }
    }

    if (view_.hoveredConsumableIndex().has_value()) {
        const std::size_t index = *view_.hoveredConsumableIndex();
        if (index < view_.model().consumables.size()) {
            const ConsumableViewModel& consumableModel = view_.model().consumables[index];
            InspectPanelModel panel;
            if (consumableModel.filled && content_.consumables().contains(ConsumableId(consumableModel.id))) {
                panel = inspectModelBuilder_.buildConsumable(content_.consumables().get(ConsumableId(consumableModel.id)));
            } else {
                panel = inspectModelBuilder_.buildConsumable(consumableModel);
            }
            const float screenWidth = static_cast<float>(VirtualViewport::width());
            const float screenHeight = static_cast<float>(VirtualViewport::height());
            constexpr float screenMargin = 18.f;
            const float width = std::min(420.f, screenWidth - screenMargin * 2.f);
            const Rectangle bounds{
                std::max(screenMargin, screenWidth - width - screenMargin),
                78.f,
                width,
                std::min(360.f, screenHeight - 120.f)
            };
            inspectPanelView_.render(uiFont_, panel, bounds);
            return;
        }
    }

    if (view_.hoveredDroneSlotIndex().has_value()) {
        const std::size_t index = *view_.hoveredDroneSlotIndex();
        if (index < view_.model().droneSlots.size()) {
            const InspectPanelModel panel = inspectModelBuilder_.buildDroneSlot(view_.model().droneSlots[index]);

            constexpr float gap = 12.f;
            constexpr float screenMargin = 18.f;
            constexpr float minWidth = 320.f;
            constexpr float preferredWidth = 460.f;

            const float screenWidth = static_cast<float>(VirtualViewport::width());
            const float screenHeight = static_cast<float>(VirtualViewport::height());
            Rectangle bounds{
                screenWidth * 0.5f - preferredWidth * 0.5f,
                136.f,
                preferredWidth,
                std::min(360.f, screenHeight - 150.f)
            };

            const std::optional<Rectangle> slotBounds = view_.hoveredDroneSlotBounds();
            if (slotBounds.has_value()) {
                const float rightX = slotBounds->x + slotBounds->width + gap;
                const float availableRightWidth = screenWidth - rightX - screenMargin;

                if (availableRightWidth >= minWidth) {
                    bounds.x = rightX;
                    bounds.width = std::clamp(availableRightWidth, minWidth, preferredWidth);
                } else {
                    const float availableLeftWidth = slotBounds->x - gap - screenMargin;
                    bounds.width = std::clamp(availableLeftWidth, minWidth, preferredWidth);
                    bounds.x = std::max(screenMargin, slotBounds->x - gap - bounds.width);
                }

                bounds.y = std::clamp(slotBounds->y, 82.f, screenHeight - bounds.height - screenMargin);
            }

            inspectPanelView_.render(uiFont_, panel, bounds);
            return;
        }
    }

    if (view_.hoveredStatus().has_value()) {
        const InspectPanelModel panel = inspectModelBuilder_.buildStatus(*view_.hoveredStatus());

        constexpr float gap = 12.f;
        constexpr float screenMargin = 18.f;
        constexpr float preferredWidth = 360.f;
        const float screenWidth = static_cast<float>(VirtualViewport::width());
        const float screenHeight = static_cast<float>(VirtualViewport::height());

        Rectangle bounds{
            screenWidth - preferredWidth - screenMargin,
            94.f,
            preferredWidth,
            std::min(360.f, screenHeight - 120.f)
        };

        const std::optional<Rectangle> statusBounds = view_.hoveredStatusBounds();
        if (statusBounds.has_value()) {
            const float rightX = statusBounds->x + statusBounds->width + gap;
            if (rightX + preferredWidth + screenMargin <= screenWidth) {
                bounds.x = rightX;
            } else {
                bounds.x = std::max(screenMargin, statusBounds->x - gap - preferredWidth);
            }
            bounds.y = std::clamp(statusBounds->y - 16.f, 82.f, screenHeight - bounds.height - screenMargin);
        }

        inspectPanelView_.render(uiFont_, panel, bounds);
        return;
    }

    const std::optional<PlayerViewModel> playerModel = hoveredPlayerViewModel();
    if (playerModel.has_value()) {
        const InspectPanelModel panel = inspectModelBuilder_.buildPlayer(*playerModel);

        constexpr float gap = 12.f;
        constexpr float screenMargin = 18.f;
        constexpr float minWidth = 220.f;
        constexpr float preferredWidth = 300.f;

        const float screenHeight = static_cast<float>(VirtualViewport::height());

        Rectangle bounds{
            screenMargin,
            94.f,
            preferredWidth,
            std::min(440.f, screenHeight - 120.f)
        };

        const std::optional<Rectangle> playerBounds = view_.hoveredPlayerBounds();
        if (playerBounds.has_value()) {
            const float availableLeftWidth = playerBounds->x - gap - screenMargin;
            bounds.width = std::clamp(availableLeftWidth, minWidth, preferredWidth);
            bounds.x = std::max(screenMargin, playerBounds->x - gap - bounds.width);
            bounds.y = std::clamp(playerBounds->y, 82.f, screenHeight - bounds.height - screenMargin);
        }

        inspectPanelView_.render(uiFont_, panel, bounds);
        return;
    }

    const std::optional<EnemyViewModel> enemyModel = hoveredEnemyViewModel();
    if (enemyModel.has_value()) {
        const InspectPanelModel panel = inspectModelBuilder_.buildEnemy(*enemyModel);

        constexpr float gap = 12.f;
        constexpr float screenMargin = 18.f;
        constexpr float minWidth = 220.f;
        constexpr float preferredWidth = 300.f;

        const float screenWidth = static_cast<float>(VirtualViewport::width());
        const float screenHeight = static_cast<float>(VirtualViewport::height());

        Rectangle bounds{
            screenWidth - preferredWidth - screenMargin,
            94.f,
            preferredWidth,
            std::min(440.f, screenHeight - 120.f)
        };

        const std::optional<Rectangle> enemyBounds = view_.hoveredEnemyBounds();
        if (enemyBounds.has_value()) {
            const float rightX = enemyBounds->x + enemyBounds->width + gap;
            const float availableRightWidth = screenWidth - rightX - screenMargin;
            bounds.width = std::clamp(availableRightWidth, minWidth, preferredWidth);
            bounds.x = std::min(rightX, screenWidth - bounds.width - screenMargin);
            bounds.x = std::max(screenMargin, bounds.x);
            bounds.y = std::clamp(enemyBounds->y, 82.f, screenHeight - bounds.height - screenMargin);
        }

        inspectPanelView_.render(uiFont_, panel, bounds);
    }
}

std::optional<EnemyViewModel> CombatScene::hoveredEnemyViewModel() const {
    if (!view_.hoveredEnemyId().has_value()) {
        return std::nullopt;
    }

    for (const EnemyViewModel& enemy : view_.model().enemies) {
        if (enemy.entityId == *view_.hoveredEnemyId()) {
            return enemy;
        }
    }

    return std::nullopt;
}

std::optional<PlayerViewModel> CombatScene::hoveredPlayerViewModel() const {
    if (!view_.hoveredPlayerId().has_value()) {
        return std::nullopt;
    }

    for (const PlayerViewModel& player : view_.model().players) {
        if (player.entityId == *view_.hoveredPlayerId()) {
            return player;
        }
    }

    return std::nullopt;
}

std::optional<CardViewModel> CombatScene::inspectedCardViewModel() const {
    if (!inspectedCardId_.has_value() || !state_.hand.contains(*inspectedCardId_)) {
        return std::nullopt;
    }

    const std::optional<EntityId> previewTarget = lastPreviewTarget_;
    return cardViewModelBuilder_.build(
        state_,
        *inspectedCardId_,
        sourceForCard(*inspectedCardId_),
        previewTarget
    );
}

CardViewModel CombatScene::cardViewModelForInstance(const CardInstance& card) const {
    CardViewModel model;
    model.instanceId = card.instanceId;

    if (!content_.cards().contains(card.definitionId)) {
        model.name = card.definitionId.value;
        model.description = card.definitionId.value;
        model.energyCost = 0;
        model.playable = false;
        model.upgraded = card.upgraded;
        return model;
    }

    const CardDefinition& baseDefinition = content_.cards().get(card.definitionId);
    const CardDefinition definition = CardUpgrade::effectiveDefinition(baseDefinition, card.upgraded);
    const CardDescriptionFormatter formatter(localization_);

    model.name = localizedOrFallback(definition.nameTextId, card.definitionId.value) + (card.upgraded ? "+" : "");
    model.description = formatter.formatStaticDescription(definition);
    if (definition.ownerActorId.empty()) {
        model.ownerLabel = localizedOrFallback(TextId("ui.card_owner.common"), "Common card");
    } else {
        std::string ownerName = definition.ownerActorId;
        if (content_.actors().contains(PlayerActorId(definition.ownerActorId))) {
            const PlayerActorDefinition& actor = content_.actors().get(PlayerActorId(definition.ownerActorId));
            ownerName = localizedOrFallback(actor.nameTextId, definition.ownerActorId);
        }

        model.ownerLabel = localization_.format(TextId("ui.card_owner.actor"), {{"actor", ownerName}});
    }
    model.energyCost = definition.energyCost;
    model.type = definition.type;
    model.rarity = definition.rarity;
    model.playable = false;
    model.upgraded = card.upgraded;
    return model;
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


bool CombatScene::combatItemInspectOpen() const {
    return inspectedRelicIndex_.has_value() || inspectedConsumableIndex_.has_value();
}

void CombatScene::openRelicInspect(const std::size_t index) {
    if (index >= view_.model().relics.size()) {
        return;
    }

    inspectedRelicIndex_ = index;
    inspectedConsumableIndex_.reset();
    inspectedCardId_.reset();
    inspectedPileCardIndex_.reset();
    relicInspectModal_.close();
    clearCardSelection();
}

void CombatScene::openConsumableInspect(const std::size_t index) {
    if (index >= view_.model().consumables.size() || !view_.model().consumables[index].filled) {
        return;
    }

    inspectedConsumableIndex_ = index;
    inspectedRelicIndex_.reset();
    inspectedCardId_.reset();
    inspectedPileCardIndex_.reset();
    relicInspectModal_.close();
    clearCardSelection();
}

void CombatScene::closeCombatItemInspect() {
    inspectedRelicIndex_.reset();
    inspectedConsumableIndex_.reset();
}

void CombatScene::updateCombatItemInspectInput(const Vector2 mousePosition) {
    if (!combatItemInspectOpen()) {
        return;
    }

    if (inspectedRelicIndex_.has_value()) {
        if (view_.model().relics.empty()) {
            closeCombatItemInspect();
            return;
        }

        if (*inspectedRelicIndex_ >= view_.model().relics.size()) {
            inspectedRelicIndex_ = view_.model().relics.size() - 1;
        }
    }

    if (inspectedConsumableIndex_.has_value()) {
        const std::size_t count = view_.model().consumables.size();
        if (*inspectedConsumableIndex_ >= count || !view_.model().consumables[*inspectedConsumableIndex_].filled) {
            const std::optional<std::size_t> next = nextFilledConsumableIndex(0, 1);
            if (!next.has_value()) {
                closeCombatItemInspect();
                return;
            }
            inspectedConsumableIndex_ = *next;
        }
    }

    const Rectangle modal = combatItemInspectModalBounds();

    if (IsKeyPressed(KEY_ESCAPE)) {
        closeCombatItemInspect();
        return;
    }

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        inspectPreviousItem();
        return;
    }

    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        inspectNextItem();
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (BasicUi::contains(combatItemInspectCloseButtonBounds(modal), mousePosition)) {
            closeCombatItemInspect();
            return;
        }

        if (BasicUi::contains(combatItemInspectPreviousButtonBounds(modal), mousePosition)) {
            inspectPreviousItem();
            return;
        }

        if (BasicUi::contains(combatItemInspectNextButtonBounds(modal), mousePosition)) {
            inspectNextItem();
            return;
        }

        if (!BasicUi::contains(modal, mousePosition)) {
            closeCombatItemInspect();
        }
    }
}

void CombatScene::renderCombatItemInspectModal() const {
    if (!combatItemInspectOpen()) {
        return;
    }

    InspectPanelModel panel;
    std::size_t itemIndex = 0;
    std::size_t itemCount = 0;

    if (inspectedRelicIndex_.has_value()) {
        itemIndex = *inspectedRelicIndex_;
        itemCount = view_.model().relics.size();
        if (itemIndex >= itemCount) {
            return;
        }

        const RelicViewModel& relicModel = view_.model().relics[itemIndex];
        if (content_.relics().contains(RelicId(relicModel.id))) {
            panel = inspectModelBuilder_.buildRelic(content_.relics().get(RelicId(relicModel.id)));
        } else {
            panel.header = relicModel.name;
            panel.subheader = relicModel.description;
        }
    } else if (inspectedConsumableIndex_.has_value()) {
        itemIndex = *inspectedConsumableIndex_;
        itemCount = view_.model().consumables.size();
        if (itemIndex >= itemCount || !view_.model().consumables[itemIndex].filled) {
            return;
        }

        const ConsumableViewModel& consumableModel = view_.model().consumables[itemIndex];
        if (content_.consumables().contains(ConsumableId(consumableModel.id))) {
            panel = inspectModelBuilder_.buildConsumable(content_.consumables().get(ConsumableId(consumableModel.id)));
        } else {
            panel = inspectModelBuilder_.buildConsumable(consumableModel);
        }
    }

    const Rectangle modal = combatItemInspectModalBounds();
    const Vector2 mouse = GetMousePosition();

    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 145});
    DrawRectangleRounded(modal, 0.045f, 14, Color{18, 20, 28, 250});
    DrawRectangleRoundedLinesEx(modal, 0.045f, 14, 3.f, Color{238, 196, 86, 255});

    const Rectangle panelBounds{
        modal.x + 24.f,
        modal.y + 24.f,
        modal.width - 48.f,
        modal.height - 104.f
    };
    inspectPanelView_.render(uiFont_, panel, panelBounds);

    const Rectangle previous = combatItemInspectPreviousButtonBounds(modal);
    const Rectangle next = combatItemInspectNextButtonBounds(modal);
    BasicUi::drawButton(uiFont_, previous, "<", mouse);
    BasicUi::drawButton(uiFont_, next, ">", mouse);
    BasicUi::drawButton(uiFont_, combatItemInspectCloseButtonBounds(modal), localizedOrFallback(TextId("ui.close"), "Close"), mouse);

    const std::string counter = itemCount > 0
        ? std::to_string(itemIndex + 1) + "/" + std::to_string(itemCount)
        : "0/0";
    BasicUi::drawCenteredText(
        uiFont_,
        counter,
        Rectangle{previous.x + previous.width + 8.f, previous.y, next.x - previous.x - previous.width - 16.f, previous.height},
        16.f,
        Color{185, 190, 205, 255}
    );
}

Rectangle CombatScene::combatItemInspectModalBounds() const {
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

Rectangle CombatScene::combatItemInspectCloseButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + modal.width - 136.f, modal.y + modal.height - 58.f, 112.f, 40.f};
}

Rectangle CombatScene::combatItemInspectPreviousButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + 24.f, modal.y + modal.height - 58.f, 52.f, 40.f};
}

Rectangle CombatScene::combatItemInspectNextButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + 86.f, modal.y + modal.height - 58.f, 52.f, 40.f};
}

void CombatScene::inspectPreviousItem() {
    if (inspectedRelicIndex_.has_value()) {
        const std::size_t count = view_.model().relics.size();
        if (count == 0) {
            closeCombatItemInspect();
            return;
        }

        inspectedRelicIndex_ = *inspectedRelicIndex_ == 0 ? count - 1 : *inspectedRelicIndex_ - 1;
        return;
    }

    if (inspectedConsumableIndex_.has_value()) {
        const std::optional<std::size_t> previous = nextFilledConsumableIndex(*inspectedConsumableIndex_, -1);
        if (previous.has_value()) {
            inspectedConsumableIndex_ = *previous;
        }
    }
}

void CombatScene::inspectNextItem() {
    if (inspectedRelicIndex_.has_value()) {
        const std::size_t count = view_.model().relics.size();
        if (count == 0) {
            closeCombatItemInspect();
            return;
        }

        inspectedRelicIndex_ = (*inspectedRelicIndex_ + 1) % count;
        return;
    }

    if (inspectedConsumableIndex_.has_value()) {
        const std::optional<std::size_t> next = nextFilledConsumableIndex(*inspectedConsumableIndex_, 1);
        if (next.has_value()) {
            inspectedConsumableIndex_ = *next;
        }
    }
}

std::optional<std::size_t> CombatScene::nextFilledConsumableIndex(const std::size_t start, const int direction) const {
    const std::size_t count = view_.model().consumables.size();
    if (count == 0) {
        return std::nullopt;
    }

    const int step = direction < 0 ? -1 : 1;
    std::size_t index = start % count;
    for (std::size_t visited = 0; visited < count; ++visited) {
        index = static_cast<std::size_t>((static_cast<int>(index) + step + static_cast<int>(count)) % static_cast<int>(count));
        if (view_.model().consumables[index].filled) {
            return index;
        }
    }

    return std::nullopt;
}

void CombatScene::openConsumableConfirmation(const std::size_t index) {
    if (index >= combatConsumableIds_.size()) {
        pendingConsumableIndex_.reset();
        targetingConsumableIndex_.reset();
        return;
    }

    pendingConsumableIndex_ = index;
    targetingConsumableIndex_.reset();
    selectedCardId_.reset();
    draggedCardId_.reset();
    keyboardTargetId_.reset();
    inspectedCardId_.reset();
    lastPreviewTarget_.reset();
}

void CombatScene::cancelConsumableConfirmation() {
    pendingConsumableIndex_.reset();
}

void CombatScene::confirmConsumableUse() {
    if (!pendingConsumableIndex_.has_value()) {
        return;
    }

    const std::size_t index = *pendingConsumableIndex_;
    pendingConsumableIndex_.reset();

    if (consumableRequiresTarget(index)) {
        startConsumableTargeting(index);
        return;
    }

    tryUseConsumable(index, std::nullopt);
}

void CombatScene::startConsumableTargeting(const std::size_t index) {
    if (index >= combatConsumableIds_.size()) {
        targetingConsumableIndex_.reset();
        return;
    }

    targetingConsumableIndex_ = index;
    selectedCardId_.reset();
    draggedCardId_.reset();
    keyboardTargetId_.reset();
    inspectedCardId_.reset();
    inspectedPileCardIndex_.reset();
    lastPreviewTarget_.reset();
    viewModelDirty_ = true;
}

void CombatScene::cancelConsumableTargeting() {
    targetingConsumableIndex_.reset();
    lastPreviewTarget_.reset();
    viewModelDirty_ = true;
}

void CombatScene::tryUseConsumable(const std::size_t index, const std::optional<EntityId> target) {
    if (index >= combatConsumableIds_.size()) {
        return;
    }

    const std::string consumableId = combatConsumableIds_[index];
    const std::optional<EntityId> explicitTarget = target.has_value() ? target : std::optional<EntityId>{primaryPlayerId()};
    if (consumableSystem_.useConsumable(
            state_,
            consumableId,
            primaryPlayerId(),
            explicitTarget,
            effectSystem_,
            random_
        )) {
        combatConsumableIds_.erase(combatConsumableIds_.begin() + static_cast<std::ptrdiff_t>(index));
        selectedCardId_.reset();
        draggedCardId_.reset();
        keyboardTargetId_.reset();
        inspectedCardId_.reset();
        inspectedPileCardIndex_.reset();
        targetingConsumableIndex_.reset();
        lastPreviewTarget_.reset();
        finalResult_ = combatController_.updateAfterAction(state_);
        if (finalResult_.outcome == CombatOutcome::Ongoing) {
            turnSystem_.refreshEnemyIntentValues(state_);
        }
        viewModelDirty_ = true;
    }
}

void CombatScene::updateConsumableConfirmationInput(const Vector2 mousePosition) {
    if (!pendingConsumableIndex_.has_value() || *pendingConsumableIndex_ >= combatConsumableIds_.size()) {
        cancelConsumableConfirmation();
        return;
    }

    const Rectangle modal = consumableConfirmationBounds();

    if (IsKeyPressed(KEY_ESCAPE)) {
        cancelConsumableConfirmation();
        return;
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        confirmConsumableUse();
        return;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }

    if (BasicUi::contains(consumableConfirmButtonBounds(modal), mousePosition)) {
        confirmConsumableUse();
        return;
    }

    if (BasicUi::contains(consumableCancelButtonBounds(modal), mousePosition) ||
        !BasicUi::contains(modal, mousePosition)) {
        cancelConsumableConfirmation();
        return;
    }
}

void CombatScene::updateConsumableTargetingInput(const Vector2) {
    if (!targetingConsumableIndex_.has_value() || *targetingConsumableIndex_ >= combatConsumableIds_.size()) {
        cancelConsumableTargeting();
        return;
    }

    const std::optional<EntityId> previewTarget = previewTargetForConsumable(*targetingConsumableIndex_);
    if (previewTarget != lastPreviewTarget_) {
        lastPreviewTarget_ = previewTarget;
        viewModelDirty_ = true;
    }

    if (IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        cancelConsumableTargeting();
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (previewTarget.has_value()) {
            tryUseConsumable(*targetingConsumableIndex_, previewTarget);
        }
        return;
    }
}

void CombatScene::renderConsumableConfirmationModal() const {
    if (!pendingConsumableIndex_.has_value() || *pendingConsumableIndex_ >= combatConsumableIds_.size()) {
        return;
    }

    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 120});

    const std::size_t index = *pendingConsumableIndex_;
    const std::string consumableId = combatConsumableIds_[index];
    std::string name = consumableId;
    std::string description;
    if (content_.consumables().contains(ConsumableId(consumableId))) {
        const ConsumableDefinition& definition = content_.consumables().get(ConsumableId(consumableId));
        name = localizedOrFallback(definition.nameTextId, consumableId);
        description = localizedOrFallback(definition.descriptionTextId, {});
    }

    const Vector2 mouse = GetMousePosition();
    const Rectangle modal = consumableConfirmationBounds();
    DrawRectangleRounded(modal, 0.045f, 14, Color{28, 30, 40, 250});
    DrawRectangleRoundedLinesEx(modal, 0.045f, 14, 3.f, Color{238, 196, 86, 255});

    BasicUi::drawCenteredText(
        uiFont_,
        localizedOrFallback(TextId("consumable.confirm.title"), "Use consumable?"),
        Rectangle{modal.x + 28.f, modal.y + 24.f, modal.width - 56.f, 36.f},
        30.f,
        Color{255, 235, 175, 255}
    );

    BasicUi::drawCenteredText(
        uiFont_,
        name,
        Rectangle{modal.x + 36.f, modal.y + 74.f, modal.width - 72.f, 32.f},
        24.f,
        Color{245, 245, 250, 255}
    );

    const std::string fallbackDescription = consumableRequiresTarget(index)
        ? localizedOrFallback(TextId("consumable.confirm.target_description"), "Confirm, then choose a highlighted target.")
        : localizedOrFallback(TextId("consumable.confirm.description"), "This will consume the item immediately.");

    const std::vector<std::string> lines = BasicUi::wrapText(
        uiFont_,
        description.empty() ? fallbackDescription : description + "\n" + fallbackDescription,
        18.f,
        modal.width - 72.f
    );

    float y = modal.y + 124.f;
    for (const std::string& line : lines) {
        if (y > modal.y + modal.height - 110.f) {
            break;
        }
        BasicUi::drawText(uiFont_, line, Vector2{modal.x + 36.f, y}, 18.f, Color{205, 210, 225, 255});
        y += 24.f;
    }

    BasicUi::drawButton(
        uiFont_,
        consumableCancelButtonBounds(modal),
        localizedOrFallback(TextId("ui.cancel"), "Cancel"),
        mouse
    );

    BasicUi::drawButton(
        uiFont_,
        consumableConfirmButtonBounds(modal),
        consumableRequiresTarget(index)
            ? localizedOrFallback(TextId("consumable.confirm.choose_target"), "Choose target")
            : localizedOrFallback(TextId("ui.confirm"), "Confirm"),
        mouse
    );
}

bool CombatScene::consumableRequiresTarget(const std::size_t index) const {
    return consumableCanTargetEnemy(index) || consumableCanTargetPlayer(index);
}

bool CombatScene::consumableCanTargetEnemy(const std::size_t index) const {
    if (index >= combatConsumableIds_.size()) {
        return false;
    }

    const ConsumableId id(combatConsumableIds_[index]);
    if (!content_.consumables().contains(id)) {
        return false;
    }

    const ConsumableDefinition& definition = content_.consumables().get(id);
    return std::any_of(definition.effects.begin(), definition.effects.end(), [](const EffectDefinition& effect) {
        return effect.target == EffectTarget::SingleEnemy;
    });
}

bool CombatScene::consumableCanTargetPlayer(const std::size_t index) const {
    if (index >= combatConsumableIds_.size()) {
        return false;
    }

    const ConsumableId id(combatConsumableIds_[index]);
    if (!content_.consumables().contains(id)) {
        return false;
    }

    const ConsumableDefinition& definition = content_.consumables().get(id);
    return std::any_of(definition.effects.begin(), definition.effects.end(), [](const EffectDefinition& effect) {
        return effect.target == EffectTarget::Ally;
    });
}

std::vector<EntityId> CombatScene::targetCandidatesForConsumable(const std::size_t index) const {
    std::vector<EntityId> candidates;

    if (consumableCanTargetEnemy(index)) {
        const std::vector<EntityId> enemies = state_.aliveEnemyIds();
        candidates.insert(candidates.end(), enemies.begin(), enemies.end());
    }

    if (consumableCanTargetPlayer(index)) {
        const std::vector<EntityId> players = state_.alivePlayerIds();
        candidates.insert(candidates.end(), players.begin(), players.end());
    }

    return candidates;
}

std::optional<EntityId> CombatScene::previewTargetForConsumable(const std::size_t index) const {
    const std::vector<EntityId> candidates = targetCandidatesForConsumable(index);
    if (candidates.empty()) {
        return std::nullopt;
    }

    const auto isCandidate = [&candidates](const EntityId id) {
        return std::find(candidates.begin(), candidates.end(), id) != candidates.end();
    };

    if (view_.hoveredEnemyId().has_value() && isCandidate(*view_.hoveredEnemyId())) {
        return view_.hoveredEnemyId();
    }

    if (view_.hoveredPlayerId().has_value() && isCandidate(*view_.hoveredPlayerId())) {
        return view_.hoveredPlayerId();
    }

    return std::nullopt;
}

Rectangle CombatScene::consumableConfirmationBounds() const {
    const float width = std::min(560.f, static_cast<float>(VirtualViewport::width()) - 72.f);
    const float height = 340.f;
    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - width * 0.5f,
        static_cast<float>(VirtualViewport::height()) * 0.5f - height * 0.5f,
        width,
        height
    };
}

Rectangle CombatScene::consumableConfirmButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + modal.width * 0.5f + 18.f, modal.y + modal.height - 70.f, 190.f, 48.f};
}

Rectangle CombatScene::consumableCancelButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + modal.width * 0.5f - 208.f, modal.y + modal.height - 70.f, 190.f, 48.f};
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

void CombatScene::playSelectedCardOn(const EntityId target) {
    pendingConsumableIndex_.reset();

    if (!selectedCardId_.has_value()) {
        return;
    }

    const CardInstanceId playedCardId = *selectedCardId_;
    const EntityId source = sourceForCard(playedCardId);
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
    } else if (pendingAnimation.has_value()) {
        playedCardAnimations_.push_back(std::move(*pendingAnimation));
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
            turnSystem_.refreshEnemyIntentValues(state_);
        }
    }

    viewModelDirty_ = true;
}


void CombatScene::updatePlayedCardAnimations(const float deltaSeconds) {
    if (playedCardAnimations_.empty()) {
        return;
    }

    PlayedCardAnimation& active = playedCardAnimations_.front();
    active.elapsedSeconds += std::max(0.f, deltaSeconds);

    const float totalDuration = active.kind == CardFlightAnimationKind::HandToDiscard ? 0.24f : 0.688f;
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
    const float t = smoothStep(animation.elapsedSeconds / 0.24f);
    const float baseScale = CardVisualInstance::standardScale();
    const Vector2 discard = discardPileCenterPosition();

    CardTransform transform;
    transform.position = lerpVector(animation.sourcePosition, discard, t);
    const float scale = baseScale * (1.0f + (0.20f - 1.0f) * t);
    transform.scale = Vector2{scale, scale};
    transform.rotationDegrees = -10.f * t;
    transform.zIndex = 8950;
    return transform;
}

void CombatScene::renderPlayedCardAnimations() const {
    if (playedCardAnimations_.empty()) {
        return;
    }

    const Font* font = uiFont_.available() ? &uiFont_.font() : nullptr;
    const std::size_t visibleQueuedCards = std::min<std::size_t>(playedCardAnimations_.size(), 5u);

    for (std::size_t i = visibleQueuedCards; i-- > 1u;) {
        const PlayedCardAnimation& animation = playedCardAnimations_[i];
        const float queueDepth = static_cast<float>(i - 1u);
        CardTransform transform;
        if (animation.kind == CardFlightAnimationKind::HandToDiscard) {
            const float scale = CardVisualInstance::standardScale() * std::max(0.70f, 0.92f - queueDepth * 0.05f);
            transform = CardTransform{
                animation.sourcePosition,
                Vector2{scale, scale},
                -3.f,
                8450 - static_cast<int>(i)
            };
        } else {
            const float scale = CardVisualInstance::standardScale() * std::max(0.58f, 0.78f - queueDepth * 0.06f);
            transform = CardTransform{
                playedCardQueuePosition(i),
                Vector2{scale, scale},
                -5.f,
                8500 - static_cast<int>(i)
            };
        }
        CardVisualInstance::renderStaticWithOverlay(animation.model, font, transform, Color{0, 0, 0, 110});
    }

    const PlayedCardAnimation& active = playedCardAnimations_.front();
    const CardTransform transform = active.kind == CardFlightAnimationKind::HandToDiscard
        ? handDiscardAnimationTransform(active)
        : playedCardAnimationTransform(active);
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
    std::size_t queued = 0u;
    visuallyDiscardingCardIds_.clear();

    for (const CardInstance& card : state_.hand.cards()) {
        if (cardRetainsOnTurnEnd(card)) {
            continue;
        }

        visuallyDiscardingCardIds_.push_back(card.instanceId);

        const EntityId source = sourceForCard(card);
        CardViewModel model = cardViewModelBuilder_.build(state_, card.instanceId, source, source);
        model.selected = false;
        model.playable = true;
        model.unplayableReason.clear();

        const std::optional<Vector2> currentCenter = view_.cardCenter(card.instanceId);
        PlayedCardAnimation animation{
            std::move(model),
            currentCenter.value_or(Vector2{playedCardCenterPosition().x, static_cast<float>(VirtualViewport::height()) - 150.f}),
            CardFlightAnimationKind::HandToDiscard,
            false,
            0.f
        };

        if (!playedCardAnimations_.empty() || queued > 0u) {
            animation.elapsedSeconds = 0.f;
        }
        playedCardAnimations_.push_back(std::move(animation));
        ++queued;
    }
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

    turnSystem_.endPlayerTurn(state_, random_);
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
        turnSystem_.endPlayerTurn(state_, random_);
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

void CombatScene::openRewardModalIfNeeded() {
    if (reward_.has_value() || rewardAccepted_) {
        return;
    }

    reward_ = createRewardOnVictory_(finalResult_);
    rewardSelection_ = RewardSelection{};
    activeRewardOptionIndex_.reset();
    selectedRewardCardIndex_.reset();
    rewardCardChoiceOpen_ = false;
}

void CombatScene::updateRewardModalInput(const Vector2 mousePosition) {
    if (!reward_.has_value() || rewardAccepted_) {
        return;
    }

    if (rewardCardChoiceOpen_) {
        updateRewardCardChoiceInput(mousePosition);
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        rewardAccepted_ = true;
        onRewardAccepted_(*reward_, rewardSelection_);
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (std::size_t i = 0; i < reward_->options.size(); ++i) {
            if (BasicUi::contains(rewardOptionRowBounds(i), mousePosition)) {
                takeRewardOption(i);
                return;
            }
        }

        if (BasicUi::contains(rewardContinueButtonBounds(), mousePosition)) {
            rewardAccepted_ = true;
            onRewardAccepted_(*reward_, rewardSelection_);
            return;
        }
    }
}


void CombatScene::renderDefeatModal() const {
    drawModalBackdrop();

    const float width = std::min(560.f, static_cast<float>(VirtualViewport::width()) - 72.f);
    const float height = 260.f;
    const Rectangle panel{
        static_cast<float>(VirtualViewport::width()) * 0.5f - width * 0.5f,
        static_cast<float>(VirtualViewport::height()) * 0.5f - height * 0.5f,
        width,
        height
    };

    DrawRectangleRounded(panel, 0.045f, 14, Color{29, 31, 42, 248});
    DrawRectangleRoundedLinesEx(panel, 0.045f, 14, 3.f, Color{180, 70, 75, 255});

    BasicUi::drawCenteredText(
        uiFont_,
        localizedOrFallback(TextId("combat.defeat.title"), "Defeat"),
        Rectangle{panel.x + 24.f, panel.y + 28.f, panel.width - 48.f, 44.f},
        34.f,
        Color{255, 210, 210, 255}
    );

    BasicUi::drawCenteredText(
        uiFont_,
        localizedOrFallback(TextId("combat.defeat.description"), "Your run has ended."),
        Rectangle{panel.x + 42.f, panel.y + 96.f, panel.width - 84.f, 56.f},
        22.f,
        Color{215, 220, 235, 255}
    );

    BasicUi::drawCenteredText(
        uiFont_,
        localizedOrFallback(TextId("combat.defeat.continue_hint"), "Press Enter, Space, or click to continue."),
        Rectangle{panel.x + 42.f, panel.y + 176.f, panel.width - 84.f, 42.f},
        18.f,
        Color{170, 178, 205, 255}
    );
}

void CombatScene::renderRewardModal() const {
    drawModalBackdrop();

    if (!reward_.has_value()) {
        return;
    }

    const Vector2 mouse = GetMousePosition();
    const Rectangle panel = rewardModalBounds();

    DrawRectangleRounded(panel, 0.045f, 14, Color{29, 31, 42, 248});
    DrawRectangleRoundedLinesEx(panel, 0.045f, 14, 3.f, Color{238, 196, 86, 255});

    BasicUi::drawCenteredText(
        uiFont_,
        localizedOrFallback(TextId("reward.title"), "Rewards"),
        Rectangle{panel.x + 24.f, panel.y + 18.f, panel.width - 48.f, 42.f},
        32.f,
        Color{255, 235, 175, 255}
    );

    BasicUi::drawText(
        uiFont_,
        localizedOrFallback(TextId("reward.optional_hint"), "Take what you want, then continue."),
        Vector2{panel.x + 38.f, panel.y + 68.f},
        18.f,
        Color{190, 196, 215, 255}
    );

    if (reward_->options.empty()) {
        BasicUi::drawCenteredText(
            uiFont_,
            localizedOrFallback(TextId("reward.no_rewards_remaining"), "No rewards remaining."),
            Rectangle{panel.x + 40.f, panel.y + 136.f, panel.width - 80.f, 56.f},
            24.f,
            Color{205, 210, 225, 255}
        );
    } else {
        for (std::size_t i = 0; i < reward_->options.size(); ++i) {
            const RewardOption& option = reward_->options[i];
            const Rectangle row = rewardOptionRowBounds(i);
            const bool hovered = BasicUi::contains(row, mouse);

            const Color fill = hovered ? Color{55, 59, 78, 255} : Color{41, 44, 58, 255};
            const Color border = hovered ? Color{238, 196, 86, 255} : Color{110, 120, 150, 255};

            DrawRectangleRounded(row, 0.08f, 10, fill);
            DrawRectangleRoundedLinesEx(row, 0.08f, 10, 2.5f, border);

            BasicUi::drawText(
                uiFont_,
                rewardOptionTitle(option),
                Vector2{row.x + 22.f, row.y + 14.f},
                23.f,
                Color{245, 245, 250, 255}
            );

            const std::string description = rewardOptionDescription(option);
            if (!description.empty()) {
                BasicUi::drawText(
                    uiFont_,
                    description,
                    Vector2{row.x + 22.f, row.y + 47.f},
                    16.f,
                    Color{190, 198, 220, 255}
                );
            }
        }

        for (std::size_t i = 0; i < reward_->options.size(); ++i) {
            const RewardOption& option = reward_->options[i];
            const Rectangle row = rewardOptionRowBounds(i);
            if (option.type == RewardOptionType::Relic && BasicUi::contains(row, mouse)) {
                renderRewardRelicInspect(option, row);
                break;
            }
        }
    }

    BasicUi::drawButton(
        uiFont_,
        rewardContinueButtonBounds(),
        localizedOrFallback(TextId("reward.continue"), "Continue"),
        mouse
    );

    if (rewardCardChoiceOpen_) {
        renderRewardCardChoiceModal();
    }
}

void CombatScene::renderRewardRelicInspect(const RewardOption& option, const Rectangle row) const {
    if (option.type != RewardOptionType::Relic || option.relicId.empty()) {
        return;
    }

    InspectPanelModel panel;
    panel.header = rewardRelicName(option.relicId);
    panel.subheader = rewardRelicDescription(option.relicId);
    panel.entries.push_back(InspectEntry{
        localizedOrFallback(TextId("inspect.relic.reward.name"), "Relic reward"),
        localizedOrFallback(TextId("inspect.relic.reward.description"), "Click the reward row to take this relic, or continue to skip it.")
    });

    constexpr float gap = 14.f;
    constexpr float screenMargin = 18.f;
    constexpr float preferredWidth = 460.f;
    const float screenWidth = static_cast<float>(VirtualViewport::width());
    const float screenHeight = static_cast<float>(VirtualViewport::height());

    Rectangle bounds{
        row.x + row.width + gap,
        row.y,
        std::min(preferredWidth, screenWidth - screenMargin * 2.f),
        std::min(380.f, screenHeight - screenMargin * 2.f)
    };

    if (bounds.x + bounds.width > screenWidth - screenMargin) {
        bounds.x = std::max(screenMargin, row.x - gap - bounds.width);
    }

    bounds.y = std::clamp(bounds.y, screenMargin, screenHeight - bounds.height - screenMargin);
    inspectPanelView_.render(uiFont_, panel, bounds);
}

void CombatScene::renderRewardCardChoiceModal() const {
    const RewardOption* option = activeRewardOption();
    if (option == nullptr || option->type != RewardOptionType::CardChoice) {
        return;
    }

    const Vector2 mouse = GetMousePosition();
    const Rectangle panel = rewardCardChoiceModalBounds();

    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 95});
    DrawRectangleRounded(panel, 0.045f, 14, Color{25, 27, 38, 252});
    DrawRectangleRoundedLinesEx(panel, 0.045f, 14, 3.f, Color{238, 196, 86, 255});

    BasicUi::drawCenteredText(
        uiFont_,
        localizedOrFallback(TextId("reward.card_choice_title"), "Choose one card"),
        Rectangle{panel.x + 24.f, panel.y + 18.f, panel.width - 48.f, 38.f},
        30.f,
        Color{255, 235, 175, 255}
    );

    for (std::size_t i = 0; i < option->cardOptions.size(); ++i) {
        const Rectangle bounds = rewardCardChoiceOptionBounds(i);
        const bool selected = selectedRewardCardIndex_.has_value() && *selectedRewardCardIndex_ == i;
        const CardId& cardId = option->cardOptions[i].cardId;
        if (!content_.cards().contains(cardId)) {
            BasicUi::drawCenteredText(uiFont_, cardId.value, bounds, 18.f, Color{245, 245, 250, 255});
            continue;
        }

        CardViewModel model = CardViewModelFactory::buildStatic(
            content_.cards().get(cardId),
            localization_,
            CardInstanceId{static_cast<std::uint64_t>(i + 1)},
            false,
            selected
        );
        const CardTransform transform = CardVisualInstance::transformForStandardSlot(bounds, static_cast<int>(i));
        CardVisualInstance::renderStatic(model, uiFont_.available() ? &uiFont_.font() : nullptr, transform);
    }

    BasicUi::drawButton(
        uiFont_,
        rewardCardChoiceCancelBounds(),
        localizedOrFallback(TextId("reward.cancel"), "Cancel"),
        mouse
    );

    BasicUi::drawButton(
        uiFont_,
        rewardCardChoiceConfirmBounds(),
        localizedOrFallback(TextId("reward.confirm"), "Confirm"),
        mouse,
        selectedRewardCardIndex_.has_value()
    );
}

void CombatScene::updateRewardCardChoiceInput(const Vector2 mousePosition) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        closeRewardCardChoice();
        return;
    }

    RewardOption* option = activeRewardOption();
    if (option == nullptr || option->type != RewardOptionType::CardChoice) {
        closeRewardCardChoice();
        return;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }

    for (std::size_t i = 0; i < option->cardOptions.size(); ++i) {
        if (BasicUi::contains(rewardCardChoiceOptionBounds(i), mousePosition)) {
            selectedRewardCardIndex_ = i;
            return;
        }
    }

    if (BasicUi::contains(rewardCardChoiceCancelBounds(), mousePosition)) {
        closeRewardCardChoice();
        return;
    }

    if (selectedRewardCardIndex_.has_value() &&
        BasicUi::contains(rewardCardChoiceConfirmBounds(), mousePosition)) {
        confirmRewardCardChoice();
        return;
    }
}

Rectangle CombatScene::rewardModalBounds() const {
    const float width = std::min(620.f, static_cast<float>(VirtualViewport::width()) - 72.f);
    const float height = std::min(460.f, static_cast<float>(VirtualViewport::height()) - 72.f);

    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - width * 0.5f,
        static_cast<float>(VirtualViewport::height()) * 0.5f - height * 0.5f,
        width,
        height
    };
}

Rectangle CombatScene::rewardOptionRowBounds(const std::size_t index) const {
    const Rectangle panel = rewardModalBounds();
    return Rectangle{
        panel.x + 38.f,
        panel.y + 112.f + static_cast<float>(index) * 78.f,
        panel.width - 76.f,
        62.f
    };
}

Rectangle CombatScene::rewardContinueButtonBounds() const {
    const Rectangle panel = rewardModalBounds();
    return Rectangle{panel.x + panel.width * 0.5f - 160.f, panel.y + panel.height - 68.f, 320.f, 50.f};
}

Rectangle CombatScene::rewardCardChoiceModalBounds() const {
    const float width = std::min(1040.f, static_cast<float>(VirtualViewport::width()) - 72.f);
    const float height = std::min(560.f, static_cast<float>(VirtualViewport::height()) - 72.f);

    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - width * 0.5f,
        static_cast<float>(VirtualViewport::height()) * 0.5f - height * 0.5f,
        width,
        height
    };
}

Rectangle CombatScene::rewardCardChoiceOptionBounds(const std::size_t index) const {
    const Rectangle panel = rewardCardChoiceModalBounds();
    const RewardOption* option = activeRewardOption();
    const std::size_t optionCount = option != nullptr
        ? std::min<std::size_t>(3, option->cardOptions.size())
        : 0;

    const Vector2 cardSize = CardVisualInstance::standardDisplaySize();
    const float cardWidth = cardSize.x + 18.f;
    const float cardHeight = cardSize.y + 26.f;
    if (optionCount == 0) {
        return Rectangle{panel.x + 40.f, panel.y + 100.f, cardWidth, cardHeight};
    }

    const float spacing = 34.f;
    const float totalWidth = cardWidth * static_cast<float>(optionCount) + spacing * static_cast<float>(optionCount - 1);
    const float startX = panel.x + panel.width * 0.5f - totalWidth * 0.5f;

    return Rectangle{
        startX + static_cast<float>(index) * (cardWidth + spacing),
        panel.y + 90.f,
        cardWidth,
        cardHeight
    };
}

Rectangle CombatScene::rewardCardChoiceCancelBounds() const {
    const Rectangle panel = rewardCardChoiceModalBounds();
    return Rectangle{panel.x + panel.width * 0.5f - 250.f, panel.y + panel.height - 66.f, 220.f, 48.f};
}

Rectangle CombatScene::rewardCardChoiceConfirmBounds() const {
    const Rectangle panel = rewardCardChoiceModalBounds();
    return Rectangle{panel.x + panel.width * 0.5f + 30.f, panel.y + panel.height - 66.f, 220.f, 48.f};
}

void CombatScene::openRewardCardChoice(const std::size_t optionIndex) {
    activeRewardOptionIndex_ = optionIndex;
    selectedRewardCardIndex_.reset();
    rewardCardChoiceOpen_ = true;
}

void CombatScene::closeRewardCardChoice() {
    activeRewardOptionIndex_.reset();
    selectedRewardCardIndex_.reset();
    rewardCardChoiceOpen_ = false;
}

void CombatScene::confirmRewardCardChoice() {
    RewardOption* option = activeRewardOption();
    if (option == nullptr || option->type != RewardOptionType::CardChoice || !selectedRewardCardIndex_.has_value()) {
        return;
    }

    const std::size_t selectedIndex = *selectedRewardCardIndex_;
    if (selectedIndex >= option->cardOptions.size()) {
        return;
    }

    rewardSelection_.selectedCardIds.push_back(option->cardOptions[selectedIndex].cardId);

    if (reward_.has_value() && activeRewardOptionIndex_.has_value() && *activeRewardOptionIndex_ < reward_->options.size()) {
        reward_->options.erase(reward_->options.begin() + static_cast<std::ptrdiff_t>(*activeRewardOptionIndex_));
    }

    closeRewardCardChoice();
}

void CombatScene::takeRewardOption(const std::size_t optionIndex) {
    if (!reward_.has_value() || optionIndex >= reward_->options.size()) {
        return;
    }

    RewardOption& option = reward_->options[optionIndex];

    switch (option.type) {
        case RewardOptionType::Gold:
            if (option.gold > 0) {
                rewardSelection_.goldTaken += option.gold;
            }
            reward_->options.erase(reward_->options.begin() + static_cast<std::ptrdiff_t>(optionIndex));
            break;

        case RewardOptionType::CardChoice:
            openRewardCardChoice(optionIndex);
            break;

        case RewardOptionType::Consumable:
            if (!option.consumableId.empty()) {
                rewardSelection_.selectedConsumableIds.push_back(option.consumableId);
            }
            reward_->options.erase(reward_->options.begin() + static_cast<std::ptrdiff_t>(optionIndex));
            break;

        case RewardOptionType::Relic:
            if (!option.relicId.empty()) {
                rewardSelection_.selectedRelicIds.push_back(option.relicId);
            }
            reward_->options.erase(reward_->options.begin() + static_cast<std::ptrdiff_t>(optionIndex));
            break;
    }
}

const RewardOption* CombatScene::activeRewardOption() const {
    if (!reward_.has_value() || !activeRewardOptionIndex_.has_value()) {
        return nullptr;
    }

    if (*activeRewardOptionIndex_ >= reward_->options.size()) {
        return nullptr;
    }

    return &reward_->options[*activeRewardOptionIndex_];
}

RewardOption* CombatScene::activeRewardOption() {
    if (!reward_.has_value() || !activeRewardOptionIndex_.has_value()) {
        return nullptr;
    }

    if (*activeRewardOptionIndex_ >= reward_->options.size()) {
        return nullptr;
    }

    return &reward_->options[*activeRewardOptionIndex_];
}

std::string CombatScene::rewardOptionTitle(const RewardOption& option) const {
    switch (option.type) {
        case RewardOptionType::Gold:
            return localization_.format(
                TextId("reward.take_gold"),
                {{"amount", std::to_string(option.gold)}}
            );

        case RewardOptionType::CardChoice:
            return localizedOrFallback(TextId("reward.take_card"), "Take a card");

        case RewardOptionType::Consumable:
            return localizedOrFallback(TextId("reward.take_consumable"), "Take consumable");

        case RewardOptionType::Relic:
            return localization_.format(
                TextId("reward.take_relic"),
                {{"relic", rewardRelicName(option.relicId)}}
            );
    }

    return {};
}

std::string CombatScene::rewardOptionDescription(const RewardOption& option) const {
    switch (option.type) {
        case RewardOptionType::Gold:
            return localizedOrFallback(TextId("reward.gold_description"), "Add this gold to your run.");

        case RewardOptionType::CardChoice:
            return localization_.format(
                TextId("reward.card_choice_description"),
                {{"count", std::to_string(option.cardOptions.size())}}
            );

        case RewardOptionType::Consumable:
            return option.consumableId;

        case RewardOptionType::Relic:
            return {};
    }

    return {};
}

std::string CombatScene::rewardCardName(const CardId& cardId) const {
    return localization_.get(content_.cards().get(cardId).nameTextId);
}

std::string CombatScene::rewardRelicName(const std::string& relicId) const {
    if (relicId.empty() || !content_.relics().contains(RelicId(relicId))) {
        return relicId;
    }

    return localization_.get(content_.relics().get(RelicId(relicId)).nameTextId);
}

std::string CombatScene::rewardRelicDescription(const std::string& relicId) const {
    if (relicId.empty() || !content_.relics().contains(RelicId(relicId))) {
        return relicId;
    }

    return localization_.get(content_.relics().get(RelicId(relicId)).descriptionTextId);
}

std::string CombatScene::rewardCardDescription(const CardId& cardId) const {
    const CardDescriptionFormatter descriptionFormatter(localization_);
    return descriptionFormatter.formatStaticDescription(content_.cards().get(cardId));
}

std::string CombatScene::localizedOrFallback(const TextId& textId, const std::string&) const {
    if (localization_.hasText(textId)) {
        return localization_.get(textId);
    }

    return textId.value;
}

Rectangle CombatScene::drawPileButtonBounds() const {
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

Rectangle CombatScene::discardPileButtonBounds() const {
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

Rectangle CombatScene::exhaustPileButtonBounds() const {
    const Rectangle discard = discardPileButtonBounds();
    constexpr float gap = 12.f;
    return Rectangle{discard.x + discard.width + gap, discard.y, discard.width, discard.height};
}

Rectangle CombatScene::energyBubbleBounds() const {
    constexpr float size = 62.f;
    constexpr float gapBelowActor = 12.f;
    constexpr float gapAboveHand = 10.f;
    const float screenHeight = static_cast<float>(VirtualViewport::height());
    const float handHeight = std::clamp(screenHeight * 0.36f, 250.f, 330.f);
    const float handTop = screenHeight - handHeight;

    if (const std::optional<Rectangle> playerBounds = view_.playerBounds(primaryPlayerId())) {
        const float centerX = playerBounds->x + playerBounds->width * 0.5f;
        const float preferredY = playerBounds->y + playerBounds->height + gapBelowActor;
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

Rectangle CombatScene::pileOverlayBounds() const {
    const float width = std::min(1180.f, static_cast<float>(VirtualViewport::width()) - 56.f);
    const float height = std::min(680.f, static_cast<float>(VirtualViewport::height()) - 56.f);
    return Rectangle{
        (static_cast<float>(VirtualViewport::width()) - width) * 0.5f,
        (static_cast<float>(VirtualViewport::height()) - height) * 0.5f,
        width,
        height
    };
}

Rectangle CombatScene::pileOverlayGridBounds(const Rectangle modal) const {
    return Rectangle{modal.x + 28.f, modal.y + 86.f, modal.width - 56.f, modal.height - 158.f};
}

Rectangle CombatScene::pileOverlayCloseButtonBounds(const Rectangle modal) const {
    return Rectangle{modal.x + modal.width - 146.f, modal.y + modal.height - 58.f, 112.f, 40.f};
}

Rectangle CombatScene::pileOverlayCardBounds(const Rectangle grid, const std::size_t index, const float scrollOffset) const {
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
float CombatScene::pileOverlayMaxScroll(const Rectangle grid, const std::size_t count) const {
    if (count == 0) {
        return 0.f;
    }

    constexpr int columns = 5;
    constexpr float gap = 20.f;
    const Vector2 cardSize = CardVisualInstance::standardDisplaySize();
    const float height = cardSize.y + 26.f;
    const std::size_t rows = (count + columns - 1) / columns;
    const float totalHeight = 28.f + static_cast<float>(rows) * height + static_cast<float>(rows > 0 ? rows - 1 : 0) * gap;
    return std::max(0.f, totalHeight - grid.height);
}

const std::vector<CardInstance>& CombatScene::activePileCards() const {
    switch (pileOverlayMode_) {
        case PileOverlayMode::DrawPile:
            return state_.deck.drawPile.cards();
        case PileOverlayMode::DiscardPile:
            return state_.deck.discardPile.cards();
        case PileOverlayMode::ExhaustPile:
            return state_.deck.exhaustPile.cards();
        case PileOverlayMode::None:
            break;
    }

    return state_.deck.drawPile.cards();
}

std::string CombatScene::activePileTitle() const {
    switch (pileOverlayMode_) {
        case PileOverlayMode::DrawPile:
            return localizedOrFallback(TextId("ui.draw_pile_view_title"), "Draw pile");
        case PileOverlayMode::DiscardPile:
            return localizedOrFallback(TextId("ui.discard_pile_view_title"), "Discard pile");
        case PileOverlayMode::ExhaustPile:
            return localizedOrFallback(TextId("ui.exhaust_pile_view_title"), "Burned cards");
        case PileOverlayMode::None:
            break;
    }

    return {};
}

void CombatScene::openPileOverlay(const PileOverlayMode mode) {
    pileOverlayMode_ = mode;
    pileOverlayScrollOffset_ = 0.f;
    inspectedPileCardIndex_.reset();
    clearCardSelection();
}

void CombatScene::closePileOverlay() {
    pileOverlayMode_ = PileOverlayMode::None;
    pileOverlayScrollOffset_ = 0.f;
    inspectedPileCardIndex_.reset();
}

void CombatScene::updatePileOverlay(const Vector2 mousePosition) {
    const Rectangle modal = pileOverlayBounds();
    const Rectangle grid = pileOverlayGridBounds(modal);
    const float wheel = GetMouseWheelMove();
    if (wheel != 0.f) {
        pileOverlayScrollOffset_ = std::clamp(
            pileOverlayScrollOffset_ - wheel * 76.f,
            0.f,
            pileOverlayMaxScroll(grid, activePileCards().size())
        );
    }

    if (inspectedPileCardIndex_.has_value() && IsKeyPressed(KEY_ESCAPE)) {
        inspectedPileCardIndex_.reset();
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE) ||
        (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && BasicUi::contains(pileOverlayCloseButtonBounds(modal), mousePosition))) {
        closePileOverlay();
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_I)) {
        inspectedPileCardIndex_ = hoveredPileCardIndex(mousePosition);
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const std::optional<std::size_t> hovered = hoveredPileCardIndex(mousePosition);
        if (hovered.has_value()) {
            inspectedPileCardIndex_ = *hovered;
            return;
        }

        if (inspectedPileCardIndex_.has_value()) {
            inspectedPileCardIndex_.reset();
        }
    }
}

void CombatScene::renderEnergyBubble() const {
    const Rectangle bounds = energyBubbleBounds();
    const int centerX = static_cast<int>(bounds.x + bounds.width * 0.5f);
    const int centerY = static_cast<int>(bounds.y + bounds.height * 0.5f);
    const float radius = bounds.width * 0.5f;

    DrawCircle(centerX, centerY, radius, Color{54, 48, 78, 245});
    DrawCircleLines(centerX, centerY, radius, Color{190, 170, 245, 255});

    BasicUi::drawCenteredText(
        uiFont_,
        std::to_string(state_.resources.energy()) + " / " + std::to_string(state_.resources.maxEnergy()),
        bounds,
        19.f,
        Color{246, 240, 255, 255}
    );
}

void CombatScene::renderPileButtons() const {
    const Vector2 mouse = GetMousePosition();
    renderEnergyBubble();
    BasicUi::drawButton(
        uiFont_,
        drawPileButtonBounds(),
        localizedOrFallback(TextId("ui.draw_pile"), "Draw") + ": " + std::to_string(state_.deck.drawPile.size()),
        mouse
    );
    BasicUi::drawButton(
        uiFont_,
        discardPileButtonBounds(),
        localizedOrFallback(TextId("ui.discard_pile"), "Discard") + ": " + std::to_string(state_.deck.discardPile.size()),
        mouse
    );
    BasicUi::drawButton(
        uiFont_,
        exhaustPileButtonBounds(),
        localizedOrFallback(TextId("ui.exhaust_pile"), "Burned") + ": " + std::to_string(state_.deck.exhaustPile.size()),
        mouse
    );
}

void CombatScene::renderPileOverlay() const {
    const Rectangle modal = pileOverlayBounds();
    const Rectangle grid = pileOverlayGridBounds(modal);
    const Vector2 mouse = GetMousePosition();

    DrawRectangle(0, 0, VirtualViewport::width(), VirtualViewport::height(), Color{0, 0, 0, 165});
    DrawRectangleRounded(modal, 0.04f, 16, Color{25, 27, 38, 252});
    DrawRectangleRoundedLinesEx(modal, 0.04f, 16, 3.f, Color{238, 196, 86, 255});
    BasicUi::drawCenteredText(uiFont_, activePileTitle(), Rectangle{modal.x + 24.f, modal.y + 20.f, modal.width - 48.f, 38.f}, 30.f, Color{255, 235, 175, 255});

    DrawRectangleRounded(grid, 0.02f, 8, Color{20, 22, 30, 255});

    const std::vector<CardInstance>& cards = activePileCards();
    if (cards.empty()) {
        BasicUi::drawCenteredText(uiFont_, localizedOrFallback(TextId("ui.empty"), "Empty"), grid, 24.f, Color{205, 210, 225, 255});
    } else {
        BeginScissorMode(static_cast<int>(grid.x), static_cast<int>(grid.y), static_cast<int>(grid.width), static_cast<int>(grid.height));
        for (std::size_t i = 0; i < cards.size(); ++i) {
            const Rectangle cell = pileOverlayCardBounds(grid, i, pileOverlayScrollOffset_);
            if (cell.y + cell.height < grid.y || cell.y > grid.y + grid.height) {
                continue;
            }
            renderPileCard(cards[i], cell);
        }
        EndScissorMode();
    }

    BasicUi::drawText(
        uiFont_,
        localizedOrFallback(TextId("ui.pile_inspect_hint"), "Left click, right click, or I: inspect card. Esc closes the top window."),
        Vector2{modal.x + 32.f, modal.y + modal.height - 48.f},
        15.f,
        Color{150, 160, 185, 255}
    );
    BasicUi::drawButton(uiFont_, pileOverlayCloseButtonBounds(modal), localizedOrFallback(TextId("ui.close"), "Close"), mouse);
    renderPileCardInspectPanel();
}

void CombatScene::renderPileCard(const CardInstance& card, const Rectangle bounds) const {
    CardViewModel model = cardViewModelForInstance(card);
    const bool hovered = BasicUi::contains(bounds, GetMousePosition());
    model.selected = hovered;
    const CardTransform transform = CardVisualInstance::transformForStandardSlot(bounds, 0);
    CardVisualInstance::renderStatic(model, uiFont_.available() ? &uiFont_.font() : nullptr, transform);
}



std::optional<std::size_t> CombatScene::hoveredPileCardIndex(const Vector2 mousePosition) const {
    if (pileOverlayMode_ == PileOverlayMode::None) {
        return std::nullopt;
    }

    const Rectangle grid = pileOverlayGridBounds(pileOverlayBounds());
    const std::vector<CardInstance>& cards = activePileCards();
    for (std::size_t i = 0; i < cards.size(); ++i) {
        const Rectangle cell = pileOverlayCardBounds(grid, i, pileOverlayScrollOffset_);
        if (cell.y + cell.height < grid.y || cell.y > grid.y + grid.height) {
            continue;
        }

        if (BasicUi::contains(cell, mousePosition)) {
            return i;
        }
    }

    return std::nullopt;
}

void CombatScene::renderPileCardInspectPanel() const {
    if (!inspectedPileCardIndex_.has_value()) {
        return;
    }

    const std::vector<CardInstance>& cards = activePileCards();
    if (*inspectedPileCardIndex_ >= cards.size()) {
        return;
    }

    const CardInstance& instance = cards[*inspectedPileCardIndex_];
    if (!content_.cards().contains(instance.definitionId)) {
        return;
    }

    const CardViewModel cardModel = cardViewModelForInstance(instance);
    const CardDefinition definition = CardUpgrade::effectiveDefinition(content_.cards().get(instance.definitionId), instance.upgraded);
    const InspectPanelModel panel = inspectModelBuilder_.buildCard(definition, cardModel);

    const Rectangle modal = pileOverlayBounds();
    const float width = std::min(540.f, modal.width - 80.f);
    const Rectangle bounds{
        modal.x + modal.width - width - 32.f,
        modal.y + 88.f,
        width,
        std::min(660.f, modal.height - 150.f)
    };

    DrawRectangleRounded(bounds, 0.055f, 10, Color{16, 18, 24, 245});
    inspectPanelView_.render(uiFont_, panel, bounds);
}
