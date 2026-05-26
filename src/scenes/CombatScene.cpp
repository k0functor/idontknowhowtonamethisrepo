#include "CombatScene.hpp"

#include "actors/PlayerActorDefinition.hpp"
#include "cards/CardDefinition.hpp"
#include "combat/CombatPhase.hpp"
#include "effects/EffectTarget.hpp"
#include "enemies/EnemyInstance.hpp"
#include "relics/RelicDefinition.hpp"
#include "consumables/ConsumableDefinition.hpp"
#include "run/RunMapNode.hpp"
#include "ui/BasicUi.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
CombatEntity makePlayerFromActor(const PlayerActorDefinition& actor, const EntityId id) {
    CombatEntity player;
    player.id = id;
    player.type = EntityType::Player;
    player.definitionId = actor.id.value;
    player.nameTextId = actor.nameTextId;
    player.health = Health(actor.maxHp);
    player.block = 0;
    return player;
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
      effectSystem_(
          effectResolver_,
          targeting_,
          damageSystem_,
          blockSystem_,
          energySystem_,
          drawSystem_,
          statusSystem_,
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
      playerTurnSystem_(drawSystem_),
      enemyTurnSystem_(enemyMoveSelector_, effectSystem_),
      turnSystem_(
          content_.enemies(),
          playerTurnSystem_,
          enemyTurnSystem_,
          enemyMoveSelector_,
          statusSystem_,
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

    if (relicInspectModal_.isOpen()) {
        relicInspectModal_.update(view_.model().relics.size());

        selectedCardId_.reset();
        draggedCardId_.reset();
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

    if (relicInspectModal_.isOpen()) {
        selectedCardId_.reset();
        draggedCardId_.reset();
        inspectedCardId_.reset();
        lastPreviewTarget_.reset();
        view_.setSelectedCard(std::nullopt);
        view_.setDraggedCard(std::nullopt, blockedMousePosition());
        view_.update(deltaSeconds, blockedMousePosition());
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        handleMousePressed(mousePosition);
        if (relicInspectModal_.isOpen()) {
            selectedCardId_.reset();
            draggedCardId_.reset();
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

    if (!combatFinished_) {
        renderInspectOverlay();
        return;
    }

    if (state_.phase == CombatPhase::Won) {
        renderRewardModal();
    } else {
        renderDefeatModal();
    }
}

void CombatScene::initializeCombat() {
    state_ = CombatState{};
    entityIds_.reset();
    cardFactory_.reset();
    selectedCardId_.reset();
    draggedCardId_.reset();
    inspectedCardId_.reset();
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
        state_.players.push_back(makePlayerFromActor(actor, entityId));
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

        state_.deck.drawPile.addTop(cardFactory_.create(cardId));
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

    model.relics = buildRelicViewModels();
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
    const std::string& cardId = card.definitionId.value;

    auto actorByDefinition = [this](const std::string& definitionId) -> std::optional<EntityId> {
        for (const CombatEntity& player : state_.players) {
            if (player.definitionId == definitionId && player.isAlive()) {
                return player.id;
            }
        }
        return std::nullopt;
    };

    if (cardId.rfind("sadist_", 0) == 0) {
        if (const std::optional<EntityId> id = actorByDefinition("sadist")) {
            return *id;
        }
    }

    if (cardId.rfind("masochist_", 0) == 0) {
        if (const std::optional<EntityId> id = actorByDefinition("masochist")) {
            return *id;
        }
    }

    if (cardId.rfind("merchant_", 0) == 0) {
        if (const std::optional<EntityId> id = actorByDefinition("bone_merchant")) {
            return *id;
        }
    }

    if (cardId.rfind("wanderer_", 0) == 0) {
        if (const std::optional<EntityId> id = actorByDefinition("wanderer")) {
            return *id;
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
            const CardDefinition& definition = content_.cards().get(instance.definitionId);
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

            bounds.x = rightX;
            bounds.width = std::clamp(availableRightWidth, minWidth, preferredWidth);
            bounds.y = std::clamp(enemyBounds->y, 82.f, screenHeight - bounds.height - screenMargin);

            if (bounds.x + bounds.width > screenWidth - screenMargin) {
                bounds.x = screenWidth - bounds.width - screenMargin;
            }
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

void CombatScene::handleMousePressed(const Vector2 mousePosition) {
    if (view_.hoveredRelicIndex().has_value()) {
        relicInspectModal_.open(*view_.hoveredRelicIndex());
        inspectedCardId_.reset();
        selectedCardId_.reset();
        draggedCardId_.reset();
        return;
    }

    if (view_.hoveredConsumableIndex().has_value()) {
        tryUseHoveredConsumable();
        return;
    }

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

void CombatScene::tryUseHoveredConsumable() {
    if (!view_.hoveredConsumableIndex().has_value()) {
        return;
    }

    const std::size_t index = *view_.hoveredConsumableIndex();
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
        inspectedCardId_.reset();
        lastPreviewTarget_.reset();
        finalResult_ = combatController_.updateAfterAction(state_);
        viewModelDirty_ = true;
    }
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
        PlayCardRequest{*selectedCardId_, sourceForCard(*selectedCardId_), target},
        random_
    );

    if (!result.played) {
        state_.log.add("Cannot play card: " + result.reason);
    }

    selectedCardId_.reset();
    draggedCardId_.reset();
    inspectedCardId_.reset();
    relicInspectModal_.close();
    lastPreviewTarget_.reset();

    if (result.played) {
        finalResult_ = combatController_.updateAfterAction(state_);
    }

    viewModelDirty_ = true;
}

void CombatScene::endPlayerTurn() {
    selectedCardId_.reset();
    draggedCardId_.reset();
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

    finalResult_ = combatController_.buildResult(state_);

    if (finalResult_.outcome != CombatOutcome::Ongoing) {
        combatFinished_ = true;
        selectedCardId_.reset();
        draggedCardId_.reset();
        inspectedCardId_.reset();
        relicInspectModal_.close();
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
    }

    return {};
}

std::string CombatScene::rewardCardName(const CardId& cardId) const {
    return localization_.get(content_.cards().get(cardId).nameTextId);
}

std::string CombatScene::rewardCardDescription(const CardId& cardId) const {
    const CardDefinition& card = content_.cards().get(cardId);

    TextFormatter::Variables variables;
    variables.emplace("damage", "?");
    variables.emplace("hp_damage", "?");
    variables.emplace("block", "?");
    variables.emplace("poison", "?");
    variables.emplace("value", "?");

    for (const EffectDefinition& effect : card.effects) {
        fillVariablesFromEffect(variables, effect);
    }

    return localization_.format(card.descriptionTextId, variables);
}

std::string CombatScene::localizedOrFallback(const TextId& textId, const std::string& fallback) const {
    if (localization_.hasText(textId)) {
        return localization_.get(textId);
    }

    return fallback;
}

std::string CombatScene::effectValueText(const EffectValue& value) {
    const std::string range = rangeToString(
        value.minimumPossibleValue(),
        value.maximumPossibleValue()
    );

    if (value.isDice()) {
        return range + " (" + ::toString(value.diceExpression()) + ")";
    }

    return range;
}

std::string CombatScene::rangeToString(const int minimum, const int maximum) {
    if (minimum == maximum) {
        return std::to_string(minimum);
    }

    return std::to_string(minimum) + "-" + std::to_string(maximum);
}

void CombatScene::fillVariablesFromEffect(
    TextFormatter::Variables& variables,
    const EffectDefinition& effect
) {
    const std::string value = effectValueText(effect.value);

    switch (effect.type) {
        case EffectType::Damage:
            variables["damage"] = value;
            variables["hp_damage"] = value;
            break;

        case EffectType::Block:
            variables["block"] = value;
            break;

        case EffectType::ApplyStatus:
            variables["value"] = value;
            if (effect.statusId.has_value()) {
                variables[*effect.statusId] = value;
            }
            break;

        default:
            variables["value"] = value;
            break;
    }
}
