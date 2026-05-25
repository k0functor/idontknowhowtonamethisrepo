#include "CombatScene.hpp"

#include "cards/CardDefinition.hpp"
#include "combat/CombatPhase.hpp"
#include "effects/EffectTarget.hpp"
#include "enemies/EnemyInstance.hpp"
#include "relics/RelicDefinition.hpp"
#include "ui/BasicUi.hpp"

#include <algorithm>

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
      relicSystem_(content_.relics()),
      damageSystem_(modifierSystem_, &eventBus_),
      blockSystem_(modifierSystem_, &eventBus_),
      statusSystem_(content_.statuses()),
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
        viewModelDirty_ = true;
    });

    initializeCombat();
}

void CombatScene::update(const float deltaSeconds) {
    const Vector2 mousePosition = GetMousePosition();

    if (combatFinished_) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (state_.phase == CombatPhase::Won) {
                onCombatWon_(finalResult_);
            } else {
                onCombatLost_(finalResult_);
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

    updateInspectInput(mousePosition);

    if (relicInspectModal_.isOpen()) {
        return;
    }

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
    renderInspectOverlay();

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
    inspectedCardId_.reset();
    relicInspectModal_.close();
    lastPreviewTarget_.reset();
    combatFinished_ = false;
    finalResult_ = CombatResult{};

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
    relicSystem_.startCombat();
    turnSystem_.startCombat(state_, random_);

    viewModelDirty_ = true;
    rebuildViewModel(std::nullopt);
    viewModelDirty_ = false;
}

void CombatScene::rebuildViewModel(const std::optional<EntityId> previewTarget) {
    CombatViewModel model = combatViewModelBuilder_.build(
        state_,
        playerId_,
        previewTarget
    );

    model.relics = buildRelicViewModels();
    view_.setModel(model);
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


void CombatScene::updateInspectInput(const Vector2 mousePosition) {
    relicInspectModal_.update(view_.model().relics.size());

    if (relicInspectModal_.isOpen()) {
        inspectedCardId_.reset();
        return;
    }

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
        playerId_,
        previewTarget
    );
}

void CombatScene::handleMousePressed(const Vector2 mousePosition) {
    if (view_.hoveredRelicIndex().has_value()) {
        relicInspectModal_.open(*view_.hoveredRelicIndex());
        inspectedCardId_.reset();
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
        viewModelDirty_ = true;
    }
}
