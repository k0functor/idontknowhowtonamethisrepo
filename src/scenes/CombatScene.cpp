#include "CombatScene.hpp"

#include "actors/PlayerActorDefinition.hpp"
#include "cards/CardDefinition.hpp"
#include "cards/CardDescriptionFormatter.hpp"
#include "cards/CardUpgrade.hpp"
#include "combat/CombatPhase.hpp"
#include "effects/EffectTarget.hpp"
#include "enemies/EnemyInstance.hpp"
#include "relics/RelicDefinition.hpp"
#include "consumables/ConsumableDefinition.hpp"
#include "run/RunMapNode.hpp"
#include "run/StressRules.hpp"
#include "ui/BasicUi.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
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
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 155});
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

std::vector<std::vector<std::string>> normalEncounters() {
    return {
        {"rat_cultist"},
        {"ash_hound"},
        {"plague_spider"},
        {"shield_bearer", "knife_acolyte"},
        {"rat_cultist", "grave_lamp"},
        {"knife_acolyte", "plague_spider"}
    };
}

std::vector<std::vector<std::string>> eliteEncounters() {
    return {
        {"grave_knight"},
        {"choir_executioner"},
        {"glass_collector"}
    };
}

std::vector<std::vector<std::string>> bossEncounters() {
    return {
        {"baron_of_ashes"}
    };
}

std::vector<std::string> chooseEncounter(
    const std::vector<std::vector<std::string>>& encounters,
    Random& random
) {
    if (encounters.empty()) {
        return {"training_dummy"};
    }

    const int index = random.rangeInclusive(0, static_cast<int>(encounters.size()) - 1);
    return encounters[static_cast<std::size_t>(index)];
}

std::vector<std::string> enemyIdsForNode(
    const RunState& runState,
    Random& random
) {
    switch (currentNodeType(runState)) {
        case RunMapNodeType::Elite:
            return chooseEncounter(eliteEncounters(), random);

        case RunMapNodeType::Boss:
            return chooseEncounter(bossEncounters(), random);

        case RunMapNodeType::Combat:
        case RunMapNodeType::Event:
        case RunMapNodeType::Shop:
        case RunMapNodeType::Chest:
        case RunMapNodeType::Rest:
            return chooseEncounter(normalEncounters(), random);
    }

    return chooseEncounter(normalEncounters(), random);
}

void addEnemyToCombat(
    CombatState& state,
    EntityIdGenerator& entityIds,
    const EnemyDatabase& enemies,
    const std::string& enemyId,
    const float hpMultiplier
) {
    if (!enemies.contains(EnemyId(enemyId))) {
        throw std::runtime_error("Unknown enemy in encounter: " + enemyId);
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
    std::function<RewardState(const CombatResult&)> createRewardOnVictory,
    std::function<void(const RewardState&, const RewardSelection&)> onRewardAccepted,
    std::function<void(const CombatResult&)> onCombatLost
)
    : content_(content),
      localization_(localization),
      random_(random),
      uiFont_(uiFont),
      runState_(runState),
      createRewardOnVictory_(std::move(createRewardOnVictory)),
      onRewardAccepted_(std::move(onRewardAccepted)),
      onCombatLost_(std::move(onCombatLost)),
      relicSystem_(content_.relics()),
      damageSystem_(modifierSystem_, &eventBus_),
      blockSystem_(modifierSystem_, &eventBus_),
      statusSystem_(content_.statuses()),
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
          cardViewModelBuilder_
      ),
      inspectModelBuilder_(content_, localization_) {
    relicSystem_.setRelics(runState_.relicIds);
    modifierSystem_.addProvider(relicSystem_);
    eventBus_.subscribe([this](const GameEvent& event) {
        relicSystem_.handleEvent(state_, event, effectSystem_, random_);

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
                state_.log.add("Sadist gains 1 Strength for hurting Masochist");
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
                state_.log.add("Masochist gains 1 Strength and 1 Dexterity after taking pain");
            }
        }

        viewModelDirty_ = true;
    });

    initializeCombat();
}

void CombatScene::update(const float deltaSeconds) {
    const Vector2 mousePosition = GetMousePosition();

    if (combatFinished_) {
        selectedCardId_.reset();
        draggedCardId_.reset();
        keyboardTargetId_.reset();
        inspectedCardId_.reset();
        relicInspectModal_.close();
        lastPreviewTarget_.reset();

        view_.setSelectedCard(std::nullopt);
        view_.setDraggedCard(std::nullopt, blockedMousePosition());
        view_.update(deltaSeconds, blockedMousePosition());

        if (state_.phase == CombatPhase::Won) {
            openRewardModalIfNeeded();
            updateRewardModalInput(mousePosition);
        } else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            onCombatLost_(finalResult_);
        }

        return;
    }

    if (pileOverlayMode_ != PileOverlayMode::None) {
        selectedCardId_.reset();
        draggedCardId_.reset();
        keyboardTargetId_.reset();
        inspectedCardId_.reset();
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
        lastPreviewTarget_.reset();

        view_.setSelectedCard(std::nullopt);
        view_.setDraggedCard(std::nullopt, blockedMousePosition());
        view_.update(deltaSeconds, blockedMousePosition());
        updateConsumableConfirmationInput(mousePosition);
        return;
    }

    if (relicInspectModal_.isOpen()) {
        relicInspectModal_.update(view_.model().relics.size());

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

    if (draggedCardId_.has_value()) {
        view_.setDraggedCard(draggedCardId_, mousePosition);
    } else {
        view_.setDraggedCard(std::nullopt, mousePosition);
    }

    view_.setSelectedCard(selectedCardId_);
    view_.update(deltaSeconds, mousePosition);

    updateInspectInput(mousePosition);
    handleKeyboardCombatInput();

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

    if (!combatFinished_) {
        renderPileButtons();
        if (pileOverlayMode_ != PileOverlayMode::None) {
            renderPileOverlay();
            return;
        }

        if (pendingConsumableIndex_.has_value()) {
            renderConsumableConfirmationModal();
            return;
        }

        renderTargetingArrow();
        renderInspectOverlay();
        return;
    }

    if (state_.phase == CombatPhase::Won) {
        renderRewardModal();
    } else {
        renderDefeatModal();
    }
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

    const std::string& command = tokens.front();

    if (command == "status" || (command == "apply" && tokens.size() >= 2 && tokens[1] == "status")) {
        const std::size_t idIndex = command == "status" ? 1u : 2u;
        if (tokens.size() <= idIndex) {
            output = "Usage: status <status_id> [amount] [player|enemy]";
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
            output = "No alive " + side + " target";
            return true;
        }

        statusSystem_.applyStatus(state_, *target, statusId, amount);
        turnSystem_.refreshEnemyIntentValues(state_);
        viewModelDirty_ = true;
        output = "Applied status '" + statusId + "' x" + std::to_string(amount) + " to " + side;
        return true;
    }

    if (command == "heal" || command == "damage") {
        const int amount = parseAmount(tokens, 1u, 0);
        if (amount <= 0) {
            output = "Usage: " + command + " <amount> [player|enemy]";
            return true;
        }

        std::string side = "player";
        if (tokens.size() > 2u) {
            side = tokens[2];
        }

        const std::optional<EntityId> target = firstTarget(side);
        if (!target.has_value()) {
            output = "No alive " + side + " target";
            return true;
        }

        CombatEntity& entity = state_.entity(*target);
        if (command == "heal") {
            entity.health.heal(amount);
            output = "Healed " + side + " for " + std::to_string(amount);
        } else {
            entity.health.takeDamage(amount);
            output = "Damaged " + side + " for " + std::to_string(amount);
            combatController_.updateAfterAction(state_);
            finishCombatIfNeeded();
        }

        viewModelDirty_ = true;
        return true;
    }

    if (command == "stress") {
        const int amount = parseAmount(tokens, 1u, 0);
        if (tokens.size() < 2u || amount == 0) {
            output = "Usage: stress <delta> [player|enemy]";
            return true;
        }

        std::string side = "player";
        if (tokens.size() > 2u) {
            side = tokens[2];
        }

        const std::optional<EntityId> target = firstTarget(side);
        if (!target.has_value()) {
            output = "No alive " + side + " target";
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
        output = "Adjusted " + side + " stress by " + std::to_string(result.applied);
        return true;
    }

    if (command == "block") {
        const int amount = parseAmount(tokens, 1u, 0);
        if (amount <= 0) {
            output = "Usage: block <amount> [player|enemy]";
            return true;
        }

        std::string side = "player";
        if (tokens.size() > 2u) {
            side = tokens[2];
        }

        const std::optional<EntityId> target = firstTarget(side);
        if (!target.has_value()) {
            output = "No alive " + side + " target";
            return true;
        }

        state_.entity(*target).block += amount;
        viewModelDirty_ = true;
        output = "Added block " + std::to_string(amount) + " to " + side;
        return true;
    }

    if (command == "energy") {
        const int amount = parseAmount(tokens, 1u, 0);
        if (amount == 0) {
            output = "Usage: energy <amount>";
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
        output = "Adjusted combat energy by " + std::to_string(amount);
        return true;
    }

    if (command == "win" && tokens.size() >= 2u && tokens[1] == "combat") {
        for (CombatEntity& enemy : state_.enemies) {
            enemy.health.setCurrent(0);
        }
        combatController_.updateAfterAction(state_);
        finishCombatIfNeeded();
        viewModelDirty_ = true;
        output = "Combat won by debug command";
        return true;
    }

    if (command == "lose" && tokens.size() >= 2u && tokens[1] == "combat") {
        for (CombatEntity& player : state_.players) {
            player.health.setCurrent(0);
        }
        combatController_.updateAfterAction(state_);
        finishCombatIfNeeded();
        viewModelDirty_ = true;
        output = "Combat lost by debug command";
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

    const std::vector<std::string> encounterEnemyIds = enemyIdsForNode(runState_, random_);
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

    for (const CardId& cardId : runState_.deckCardIds) {
        if (!content_.cards().contains(cardId)) {
            throw std::runtime_error("Run deck contains unknown card id: " + cardId.value);
        }

        const bool upgraded = std::find(
            runState_.upgradedCardIds.begin(),
            runState_.upgradedCardIds.end(),
            cardId
        ) != runState_.upgradedCardIds.end();
        state_.deck.drawPile.addTop(cardFactory_.create(cardId, upgraded));
    }

    state_.log.add("Combat started");
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

    if (isSadistMasochistParty()) {
        model.turnOrderLabel = localizedOrFallback(TextId("ui.turn_order.sadist_masochist"), "Turn order: Sadist -> Masochist -> Enemy");
    }

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
            slot.filled = true;
            slot.type = state_.droneSlots[i].droneId;

            const DroneId droneId(slot.type);
            if (content_.drones().contains(droneId)) {
                const DroneDefinition& definition = content_.drones().get(droneId);
                slot.name = localizedOrFallback(definition.nameTextId, slot.type);
                slot.description = localizedOrFallback(definition.descriptionTextId, slot.type);
            } else {
                slot.name = slot.type;
                slot.description = localizedOrFallback(TextId("drone.unknown.description"), slot.type);
            }
        } else {
            slot.filled = false;
            slot.type = {};
            slot.name = localizedOrFallback(TextId("drone.empty"), "Empty");
            slot.description = localizedOrFallback(TextId("drone.empty.description"), "This drone slot is empty.");
        }

        result.push_back(std::move(slot));
    }

    return result;
}

void CombatScene::updateInspectInput(const Vector2) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        inspectedCardId_.reset();
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && view_.hoveredCardId().has_value()) {
        inspectedCardId_ = *view_.hoveredCardId();
        return;
    }

    if (IsKeyPressed(KEY_I) && selectedCardId_.has_value()) {
        inspectedCardId_ = selectedCardId_;
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (!view_.hoveredCardId().has_value()) {
            inspectedCardId_.reset();
        }
    }
}

void CombatScene::renderInspectOverlay() const {
    if (relicInspectModal_.isOpen()) {
        relicInspectModal_.render(uiFont_, view_.model().relics);
        return;
    }

    if (inspectedCardId_.has_value()) {
        const std::optional<CardViewModel> cardModel = inspectedCardViewModel();
        if (cardModel.has_value() && state_.hand.contains(*inspectedCardId_)) {
            const CardInstance& instance = state_.hand.get(*inspectedCardId_);
            const CardDefinition definition = CardUpgrade::effectiveDefinition(content_.cards().get(instance.definitionId), instance.upgraded);
            const InspectPanelModel panel = inspectModelBuilder_.buildCard(definition, *cardModel);

            const float width = std::min(440.f, static_cast<float>(GetScreenWidth()) - 60.f);
            const Rectangle bounds{
                static_cast<float>(GetScreenWidth()) - width - 24.f,
                94.f,
                width,
                std::min(520.f, static_cast<float>(GetScreenHeight()) - 150.f)
            };
            inspectPanelView_.render(uiFont_, panel, bounds);
        }
        return;
    }

    if (view_.hoveredConsumableIndex().has_value()) {
        const std::size_t index = *view_.hoveredConsumableIndex();
        if (index < view_.model().consumables.size()) {
            const InspectPanelModel panel = inspectModelBuilder_.buildConsumable(view_.model().consumables[index]);
            const float screenWidth = static_cast<float>(GetScreenWidth());
            const float screenHeight = static_cast<float>(GetScreenHeight());
            constexpr float screenMargin = 18.f;
            const float width = std::min(320.f, screenWidth - screenMargin * 2.f);
            const Rectangle bounds{
                std::max(screenMargin, screenWidth - width - screenMargin),
                78.f,
                width,
                std::min(240.f, screenHeight - 120.f)
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
            constexpr float minWidth = 220.f;
            constexpr float preferredWidth = 300.f;

            const float screenWidth = static_cast<float>(GetScreenWidth());
            const float screenHeight = static_cast<float>(GetScreenHeight());
            Rectangle bounds{
                screenWidth * 0.5f - preferredWidth * 0.5f,
                136.f,
                preferredWidth,
                std::min(230.f, screenHeight - 160.f)
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

    const std::optional<PlayerViewModel> playerModel = hoveredPlayerViewModel();
    if (playerModel.has_value()) {
        const InspectPanelModel panel = inspectModelBuilder_.buildPlayer(*playerModel);

        constexpr float gap = 12.f;
        constexpr float screenMargin = 18.f;
        constexpr float minWidth = 220.f;
        constexpr float preferredWidth = 300.f;

        const float screenHeight = static_cast<float>(GetScreenHeight());

        Rectangle bounds{
            screenMargin,
            94.f,
            preferredWidth,
            std::min(300.f, screenHeight - 140.f)
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

        const float screenWidth = static_cast<float>(GetScreenWidth());
        const float screenHeight = static_cast<float>(GetScreenHeight());

        Rectangle bounds{
            screenWidth - preferredWidth - screenMargin,
            94.f,
            preferredWidth,
            std::min(300.f, screenHeight - 140.f)
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
        relicInspectModal_.open(*view_.hoveredRelicIndex());
        inspectedCardId_.reset();
        selectedCardId_.reset();
        draggedCardId_.reset();
        keyboardTargetId_.reset();
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

void CombatScene::openConsumableConfirmation(const std::size_t index) {
    if (index >= combatConsumableIds_.size()) {
        pendingConsumableIndex_.reset();
        return;
    }

    pendingConsumableIndex_ = index;
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
    tryUseConsumable(index);
}

void CombatScene::tryUseConsumable(const std::size_t index) {
    if (index >= combatConsumableIds_.size()) {
        return;
    }

    const std::string consumableId = combatConsumableIds_[index];
    if (consumableSystem_.useConsumable(
            state_,
            consumableId,
            primaryPlayerId(),
            effectSystem_,
            random_
        )) {
        combatConsumableIds_.erase(combatConsumableIds_.begin() + static_cast<std::ptrdiff_t>(index));
        selectedCardId_.reset();
        draggedCardId_.reset();
        keyboardTargetId_.reset();
        inspectedCardId_.reset();
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

void CombatScene::renderConsumableConfirmationModal() const {
    if (!pendingConsumableIndex_.has_value() || *pendingConsumableIndex_ >= combatConsumableIds_.size()) {
        return;
    }

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 120});

    const std::string consumableId = combatConsumableIds_[*pendingConsumableIndex_];
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

    const std::vector<std::string> lines = BasicUi::wrapText(
        uiFont_,
        description.empty()
            ? localizedOrFallback(TextId("consumable.confirm.description"), "This will consume the item immediately.")
            : description,
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
        localizedOrFallback(TextId("ui.confirm"), "Confirm"),
        mouse
    );
}

Rectangle CombatScene::consumableConfirmationBounds() const {
    const float width = std::min(520.f, static_cast<float>(GetScreenWidth()) - 72.f);
    const float height = 300.f;
    return Rectangle{
        static_cast<float>(GetScreenWidth()) * 0.5f - width * 0.5f,
        static_cast<float>(GetScreenHeight()) * 0.5f - height * 0.5f,
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

    const PlayCardResult result = cardPlaySystem_.playCard(
        state_,
        PlayCardRequest{*selectedCardId_, sourceForCard(*selectedCardId_), target},
        random_
    );

    if (!result.played) {
        state_.log.add("Cannot play card: " + result.reason);
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

void CombatScene::endPlayerTurn() {
    pendingConsumableIndex_.reset();
    selectedCardId_.reset();
    draggedCardId_.reset();
    keyboardTargetId_.reset();
    inspectedCardId_.reset();
    relicInspectModal_.close();
    lastPreviewTarget_.reset();
    turnSystem_.endPlayerTurn(state_, random_);
    finalResult_ = combatController_.updateAfterAction(state_);
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

void CombatScene::finishCombatIfNeeded() {
    if (combatFinished_) {
        return;
    }

    finalResult_ = combatController_.buildResult(state_);
    finalResult_.remainingConsumableIds = combatConsumableIds_;

    if (finalResult_.outcome != CombatOutcome::Ongoing) {
        combatFinished_ = true;
        selectedCardId_.reset();
        draggedCardId_.reset();
        keyboardTargetId_.reset();
        inspectedCardId_.reset();
        relicInspectModal_.close();
        pendingConsumableIndex_.reset();
        lastPreviewTarget_.reset();
        viewModelDirty_ = true;

        if (finalResult_.outcome == CombatOutcome::Victory) {
            openRewardModalIfNeeded();
        }
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

    const float width = std::min(560.f, static_cast<float>(GetScreenWidth()) - 72.f);
    const float height = 260.f;
    const Rectangle panel{
        static_cast<float>(GetScreenWidth()) * 0.5f - width * 0.5f,
        static_cast<float>(GetScreenHeight()) * 0.5f - height * 0.5f,
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
    constexpr float preferredWidth = 340.f;
    const float screenWidth = static_cast<float>(GetScreenWidth());
    const float screenHeight = static_cast<float>(GetScreenHeight());

    Rectangle bounds{
        row.x + row.width + gap,
        row.y,
        std::min(preferredWidth, screenWidth - screenMargin * 2.f),
        std::min(260.f, screenHeight - screenMargin * 2.f)
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

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 95});
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
        const bool hovered = BasicUi::contains(bounds, mouse);
        const bool selected = selectedRewardCardIndex_.has_value() && *selectedRewardCardIndex_ == i;

        const Color fill = hovered ? Color{53, 58, 75, 255} : Color{40, 43, 56, 255};
        const Color border = selected ? Color{255, 218, 90, 255} : Color{110, 120, 150, 255};

        DrawRectangleRounded(bounds, 0.08f, 10, fill);
        DrawRectangleRoundedLinesEx(bounds, 0.08f, 10, selected ? 4.f : 2.f, border);

        const CardId& cardId = option->cardOptions[i].cardId;
        BasicUi::drawCenteredText(
            uiFont_,
            rewardCardName(cardId),
            Rectangle{bounds.x + 10.f, bounds.y + 14.f, bounds.width - 20.f, 38.f},
            22.f,
            Color{245, 245, 250, 255}
        );

        const CardDefinition& definition = content_.cards().get(cardId);
        const std::string meta = std::to_string(definition.energyCost) + " energy / " + toString(definition.rarity);
        BasicUi::drawCenteredText(
            uiFont_,
            meta,
            Rectangle{bounds.x + 10.f, bounds.y + 52.f, bounds.width - 20.f, 22.f},
            14.f,
            Color{178, 184, 205, 255}
        );

        const std::vector<std::string> lines = BasicUi::wrapText(
            uiFont_,
            rewardCardDescription(cardId),
            15.f,
            bounds.width - 28.f
        );

        float y = bounds.y + 88.f;
        for (const std::string& line : lines) {
            if (y > bounds.y + bounds.height - 24.f) {
                break;
            }

            BasicUi::drawText(
                uiFont_,
                line,
                Vector2{bounds.x + 16.f, y},
                15.f,
                Color{205, 210, 225, 255}
            );
            y += 20.f;
        }
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
    const float width = std::min(620.f, static_cast<float>(GetScreenWidth()) - 72.f);
    const float height = std::min(460.f, static_cast<float>(GetScreenHeight()) - 72.f);

    return Rectangle{
        static_cast<float>(GetScreenWidth()) * 0.5f - width * 0.5f,
        static_cast<float>(GetScreenHeight()) * 0.5f - height * 0.5f,
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
    const float width = std::min(1040.f, static_cast<float>(GetScreenWidth()) - 72.f);
    const float height = std::min(560.f, static_cast<float>(GetScreenHeight()) - 72.f);

    return Rectangle{
        static_cast<float>(GetScreenWidth()) * 0.5f - width * 0.5f,
        static_cast<float>(GetScreenHeight()) * 0.5f - height * 0.5f,
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

    if (optionCount == 0) {
        return Rectangle{panel.x + 40.f, panel.y + 100.f, 260.f, 220.f};
    }

    const float spacing = 24.f;
    const float availableWidth = panel.width - 80.f;
    const float cardWidth = std::min(285.f, (availableWidth - spacing * static_cast<float>(optionCount - 1)) / static_cast<float>(optionCount));
    const float cardHeight = std::min(260.f, panel.height - 210.f);
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
            return rewardRelicDescription(option.relicId);
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

std::string CombatScene::localizedOrFallback(const TextId& textId, const std::string& fallback) const {
    if (localization_.hasText(textId)) {
        return localization_.get(textId);
    }

    return fallback;
}

Rectangle CombatScene::drawPileButtonBounds() const {
    const float width = 126.f;
    const float height = 34.f;
    const float x = static_cast<float>(GetScreenWidth()) * 0.5f - width * 1.5f - 16.f;
    return Rectangle{x, 40.f, width, height};
}

Rectangle CombatScene::discardPileButtonBounds() const {
    const Rectangle draw = drawPileButtonBounds();
    return Rectangle{draw.x + draw.width + 16.f, draw.y, draw.width, draw.height};
}

Rectangle CombatScene::exhaustPileButtonBounds() const {
    const Rectangle discard = discardPileButtonBounds();
    return Rectangle{discard.x + discard.width + 16.f, discard.y, discard.width, discard.height};
}

Rectangle CombatScene::pileOverlayBounds() const {
    const float width = std::min(1180.f, static_cast<float>(GetScreenWidth()) - 56.f);
    const float height = std::min(680.f, static_cast<float>(GetScreenHeight()) - 56.f);
    return Rectangle{
        (static_cast<float>(GetScreenWidth()) - width) * 0.5f,
        (static_cast<float>(GetScreenHeight()) - height) * 0.5f,
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
    constexpr float gap = 14.f;
    const float width = (grid.width - gap * static_cast<float>(columns - 1)) / static_cast<float>(columns);
    const float height = 210.f;
    const int column = static_cast<int>(index % columns);
    const int row = static_cast<int>(index / columns);
    return Rectangle{
        grid.x + static_cast<float>(column) * (width + gap),
        grid.y + static_cast<float>(row) * (height + gap) - scrollOffset,
        width,
        height
    };
}

float CombatScene::pileOverlayMaxScroll(const Rectangle grid, const std::size_t count) const {
    if (count == 0) {
        return 0.f;
    }

    constexpr int columns = 5;
    constexpr float gap = 14.f;
    constexpr float height = 210.f;
    const std::size_t rows = (count + columns - 1) / columns;
    const float totalHeight = static_cast<float>(rows) * height + static_cast<float>(rows > 0 ? rows - 1 : 0) * gap;
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
    clearCardSelection();
}

void CombatScene::closePileOverlay() {
    pileOverlayMode_ = PileOverlayMode::None;
    pileOverlayScrollOffset_ = 0.f;
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

    if (IsKeyPressed(KEY_ESCAPE) ||
        (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && BasicUi::contains(pileOverlayCloseButtonBounds(modal), mousePosition))) {
        closePileOverlay();
    }
}

void CombatScene::renderPileButtons() const {
    const Vector2 mouse = GetMousePosition();
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

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 165});
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

    BasicUi::drawButton(uiFont_, pileOverlayCloseButtonBounds(modal), localizedOrFallback(TextId("ui.close"), "Close"), mouse);
}

void CombatScene::renderPileCard(const CardInstance& card, const Rectangle bounds) const {
    const bool hovered = BasicUi::contains(bounds, GetMousePosition());
    DrawRectangleRounded(bounds, 0.07f, 9, hovered ? Color{52, 56, 73, 255} : Color{39, 42, 55, 255});
    DrawRectangleRoundedLinesEx(bounds, 0.07f, 9, 2.f, hovered ? Color{238, 196, 86, 255} : Color{110, 120, 150, 255});

    if (!content_.cards().contains(card.definitionId)) {
        BasicUi::drawCenteredText(uiFont_, card.definitionId.value, bounds, 16.f, Color{245, 245, 250, 255});
        return;
    }

    const CardDefinition definition = CardUpgrade::effectiveDefinition(content_.cards().get(card.definitionId), card.upgraded);
    const CardDescriptionFormatter formatter(localization_);
    BasicUi::drawText(uiFont_, std::to_string(definition.energyCost), Vector2{bounds.x + 13.f, bounds.y + 10.f}, 18.f, Color{245, 245, 250, 255});
    BasicUi::drawCenteredText(uiFont_, localization_.get(definition.nameTextId) + (card.upgraded ? "+" : ""), Rectangle{bounds.x + 38.f, bounds.y + 8.f, bounds.width - 48.f, 40.f}, 16.f, Color{245, 245, 250, 255});

    const std::vector<std::string> lines = BasicUi::wrapText(uiFont_, formatter.formatStaticDescription(definition), 12.f, bounds.width - 22.f);
    float y = bounds.y + 66.f;
    for (const std::string& line : lines) {
        if (y > bounds.y + bounds.height - 18.f) break;
        BasicUi::drawText(uiFont_, line, Vector2{bounds.x + 12.f, y}, 12.f, Color{205, 210, 225, 255});
        y += 16.f;
    }
}
