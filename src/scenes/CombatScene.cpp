#include "CombatScene.hpp"

#include "cards/CardDefinition.hpp"
#include "combat/CombatPhase.hpp"
#include "effects/EffectTarget.hpp"
#include "enemies/EnemyInstance.hpp"
#include "relics/RelicDefinition.hpp"
#include "ui/BasicUi.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

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

Vector2 blockedMousePosition() {
    return Vector2{-100000.f, -100000.f};
}

void drawModalBackdrop() {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 155});
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
    reward_.reset();
    selectedRewardCardIndex_.reset();
    rewardGoldTaken_ = false;
    rewardCardChooserOpen_ = false;
    rewardAccepted_ = false;

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
        playerId_,
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
    selectedRewardCardIndex_.reset();
    rewardGoldTaken_ = reward_->gold <= 0;
    rewardCardChooserOpen_ = false;
}

void CombatScene::updateRewardModalInput(const Vector2 mousePosition) {
    if (!reward_.has_value() || rewardAccepted_) {
        return;
    }

    if (rewardCardChooserOpen_) {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
            rewardCardChooserOpen_ = false;
            return;
        }

        if (BasicUi::contains(rewardCardCancelButtonBounds(), mousePosition) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            rewardCardChooserOpen_ = false;
            return;
        }

        for (std::size_t i = 0; i < reward_->cardOptions.size(); ++i) {
            if (BasicUi::contains(rewardCardOptionBounds(i), mousePosition) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                selectedRewardCardIndex_ = i;
                rewardCardChooserOpen_ = false;
                return;
            }
        }

        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        return;
    }

    if (!rewardGoldTaken_ && reward_->gold > 0 &&
        BasicUi::contains(rewardGoldRowBounds(), mousePosition) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        rewardGoldTaken_ = true;
        return;
    }

    if (!reward_->cardOptions.empty() &&
        BasicUi::contains(rewardCardRowBounds(), mousePosition) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        rewardCardChooserOpen_ = true;
        return;
    }

    if (BasicUi::contains(rewardContinueButtonBounds(), mousePosition) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        RewardSelection selection;
        selection.takeGold = rewardGoldTaken_;
        selection.skippedCardReward = !selectedRewardCardIndex_.has_value();

        if (selectedRewardCardIndex_.has_value()) {
            selection.selectedCardId = reward_->cardOptions[*selectedRewardCardIndex_].cardId;
        }

        rewardAccepted_ = true;
        onRewardAccepted_(*reward_, selection);
    }
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
        localizedOrFallback(TextId("reward.optional_hint"), "All rewards are optional. Take what you want, then continue."),
        Vector2{panel.x + 38.f, panel.y + 68.f},
        18.f,
        Color{190, 197, 215, 255}
    );

    const Rectangle goldRow = rewardGoldRowBounds();
    DrawRectangleRounded(goldRow, 0.06f, 10, Color{38, 41, 54, 255});
    DrawRectangleRoundedLinesEx(
        goldRow,
        0.06f,
        10,
        2.f,
        rewardGoldTaken_ ? Color{238, 196, 86, 255} : Color{92, 101, 128, 255}
    );

    BasicUi::drawText(
        uiFont_,
        localization_.format(TextId("reward.gold"), {{"amount", std::to_string(reward_->gold)}}),
        Vector2{goldRow.x + 22.f, goldRow.y + 18.f},
        24.f,
        Color{238, 226, 150, 255}
    );

    BasicUi::drawButton(
        uiFont_,
        Rectangle{goldRow.x + goldRow.width - 190.f, goldRow.y + 12.f, 166.f, 44.f},
        rewardGoldTaken_
            ? localizedOrFallback(TextId("reward.collected"), "Collected")
            : localizedOrFallback(TextId("reward.collect"), "Take"),
        mouse,
        !rewardGoldTaken_ && reward_->gold > 0,
        BasicUi::ButtonStyle{
            Color{50, 54, 70, 255},
            Color{72, 78, 96, 255},
            Color{35, 37, 46, 255},
            rewardGoldTaken_ ? Color{238, 196, 86, 255} : Color{110, 120, 150, 255},
            Color{245, 245, 250, 255},
            Color{165, 170, 184, 255}
        }
    );

    const Rectangle cardRow = rewardCardRowBounds();
    DrawRectangleRounded(cardRow, 0.06f, 10, Color{38, 41, 54, 255});
    DrawRectangleRoundedLinesEx(
        cardRow,
        0.06f,
        10,
        2.f,
        selectedRewardCardIndex_.has_value() ? Color{238, 196, 86, 255} : Color{92, 101, 128, 255}
    );

    std::string cardRowTitle = localizedOrFallback(TextId("reward.take_card"), "Take a card");
    std::string cardRowSubtitle = reward_->cardOptions.empty()
        ? localizedOrFallback(TextId("reward.no_card_options"), "No card reward.")
        : localizedOrFallback(TextId("reward.card_optional"), "Choose one of three cards or skip this reward.");

    if (selectedRewardCardIndex_.has_value()) {
        const CardId& cardId = reward_->cardOptions[*selectedRewardCardIndex_].cardId;
        cardRowSubtitle = localization_.format(
            TextId("reward.card_selected"),
            {{"card", rewardCardName(cardId)}}
        );
    }

    BasicUi::drawText(
        uiFont_,
        cardRowTitle,
        Vector2{cardRow.x + 22.f, cardRow.y + 12.f},
        24.f,
        Color{232, 236, 248, 255}
    );

    BasicUi::drawText(
        uiFont_,
        cardRowSubtitle,
        Vector2{cardRow.x + 22.f, cardRow.y + 43.f},
        17.f,
        Color{185, 194, 215, 255}
    );

    BasicUi::drawButton(
        uiFont_,
        Rectangle{cardRow.x + cardRow.width - 190.f, cardRow.y + 18.f, 166.f, 44.f},
        selectedRewardCardIndex_.has_value()
            ? localizedOrFallback(TextId("reward.change"), "Change")
            : localizedOrFallback(TextId("reward.choose"), "Choose"),
        mouse,
        !reward_->cardOptions.empty()
    );

    const Rectangle consumableRow = rewardConsumableRowBounds(0);
    DrawRectangleRounded(consumableRow, 0.06f, 10, Color{32, 34, 44, 255});
    DrawRectangleRoundedLinesEx(consumableRow, 0.06f, 10, 2.f, Color{70, 78, 98, 255});
    BasicUi::drawText(
        uiFont_,
        localizedOrFallback(TextId("reward.consumables_none"), "Consumables: none"),
        Vector2{consumableRow.x + 22.f, consumableRow.y + 20.f},
        22.f,
        Color{145, 153, 175, 255}
    );

    BasicUi::drawButton(
        uiFont_,
        rewardContinueButtonBounds(),
        localizedOrFallback(TextId("reward.continue"), "Continue"),
        mouse
    );

    if (!rewardCardChooserOpen_) {
        return;
    }

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 115});

    const Rectangle chooser = rewardCardChoiceModalBounds();
    DrawRectangleRounded(chooser, 0.045f, 14, Color{25, 27, 38, 252});
    DrawRectangleRoundedLinesEx(chooser, 0.045f, 14, 3.f, Color{238, 196, 86, 255});

    BasicUi::drawCenteredText(
        uiFont_,
        localizedOrFallback(TextId("reward.card_choice_title"), "Choose one card"),
        Rectangle{chooser.x + 24.f, chooser.y + 18.f, chooser.width - 48.f, 42.f},
        30.f,
        Color{255, 235, 175, 255}
    );

    for (std::size_t i = 0; i < reward_->cardOptions.size(); ++i) {
        const Rectangle bounds = rewardCardOptionBounds(i);
        const bool hovered = BasicUi::contains(bounds, mouse);
        const bool selected = selectedRewardCardIndex_.has_value() && *selectedRewardCardIndex_ == i;

        const Color fill = hovered ? Color{53, 58, 75, 255} : Color{40, 43, 56, 255};
        const Color border = selected ? Color{255, 218, 90, 255} : Color{110, 120, 150, 255};

        DrawRectangleRounded(bounds, 0.08f, 10, fill);
        DrawRectangleRoundedLinesEx(bounds, 0.08f, 10, selected ? 4.f : 2.f, border);

        const CardId& cardId = reward_->cardOptions[i].cardId;
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
        rewardCardCancelButtonBounds(),
        localizedOrFallback(TextId("reward.cancel"), "Cancel"),
        mouse,
        true,
        BasicUi::ButtonStyle{
            Color{42, 43, 50, 255},
            Color{60, 62, 72, 255},
            Color{35, 36, 42, 255},
            Color{120, 130, 160, 255},
            Color{235, 235, 242, 255},
            Color{120, 124, 140, 255}
        }
    );
}

void CombatScene::renderDefeatModal() const {
    drawModalBackdrop();

    const float screenWidth = static_cast<float>(GetScreenWidth());
    const float screenHeight = static_cast<float>(GetScreenHeight());
    const Rectangle bounds{
        screenWidth * 0.5f - 280.f,
        screenHeight * 0.5f - 110.f,
        560.f,
        220.f
    };

    DrawRectangleRounded(bounds, 0.08f, 12, Color{28, 26, 34, 248});
    DrawRectangleRoundedLinesEx(bounds, 0.08f, 12, 3.f, Color{190, 80, 85, 255});

    BasicUi::drawCenteredText(
        uiFont_,
        "Поражение",
        Rectangle{bounds.x + 24.f, bounds.y + 42.f, bounds.width - 48.f, 52.f},
        34.f,
        Color{255, 210, 210, 255}
    );

    BasicUi::drawCenteredText(
        uiFont_,
        "Нажми Enter, Space или ЛКМ, чтобы вернуться.",
        Rectangle{bounds.x + 36.f, bounds.y + 118.f, bounds.width - 72.f, 44.f},
        18.f,
        Color{220, 224, 235, 255}
    );
}

Rectangle CombatScene::rewardModalBounds() const {
    const float width = std::min(760.f, static_cast<float>(GetScreenWidth()) - 72.f);
    const float height = std::min(470.f, static_cast<float>(GetScreenHeight()) - 72.f);

    return Rectangle{
        static_cast<float>(GetScreenWidth()) * 0.5f - width * 0.5f,
        static_cast<float>(GetScreenHeight()) * 0.5f - height * 0.5f,
        width,
        height
    };
}

Rectangle CombatScene::rewardGoldRowBounds() const {
    const Rectangle panel = rewardModalBounds();
    return Rectangle{panel.x + 38.f, panel.y + 106.f, panel.width - 76.f, 68.f};
}

Rectangle CombatScene::rewardCardRowBounds() const {
    const Rectangle panel = rewardModalBounds();
    return Rectangle{panel.x + 38.f, panel.y + 190.f, panel.width - 76.f, 78.f};
}

Rectangle CombatScene::rewardConsumableRowBounds(const std::size_t index) const {
    const Rectangle panel = rewardModalBounds();
    return Rectangle{
        panel.x + 38.f,
        panel.y + 284.f + static_cast<float>(index) * 72.f,
        panel.width - 76.f,
        64.f
    };
}

Rectangle CombatScene::rewardContinueButtonBounds() const {
    const Rectangle panel = rewardModalBounds();
    return Rectangle{panel.x + panel.width * 0.5f - 190.f, panel.y + panel.height - 68.f, 380.f, 52.f};
}

Rectangle CombatScene::rewardCardChoiceModalBounds() const {
    const float width = std::min(1080.f, static_cast<float>(GetScreenWidth()) - 72.f);
    const float height = std::min(560.f, static_cast<float>(GetScreenHeight()) - 72.f);

    return Rectangle{
        static_cast<float>(GetScreenWidth()) * 0.5f - width * 0.5f,
        static_cast<float>(GetScreenHeight()) * 0.5f - height * 0.5f,
        width,
        height
    };
}

Rectangle CombatScene::rewardCardOptionBounds(const std::size_t index) const {
    const Rectangle panel = rewardCardChoiceModalBounds();
    const std::size_t optionCount = reward_.has_value()
        ? std::min<std::size_t>(3, reward_->cardOptions.size())
        : 0;

    if (optionCount == 0) {
        return Rectangle{panel.x + 40.f, panel.y + 120.f, 260.f, 220.f};
    }

    const float spacing = 24.f;
    const float availableWidth = panel.width - 80.f;
    const float cardWidth = std::min(285.f, (availableWidth - spacing * static_cast<float>(optionCount - 1)) / static_cast<float>(optionCount));
    const float cardHeight = std::min(285.f, panel.height - 220.f);
    const float totalWidth = cardWidth * static_cast<float>(optionCount) + spacing * static_cast<float>(optionCount - 1);
    const float startX = panel.x + panel.width * 0.5f - totalWidth * 0.5f;

    return Rectangle{
        startX + static_cast<float>(index) * (cardWidth + spacing),
        panel.y + 88.f,
        cardWidth,
        cardHeight
    };
}

Rectangle CombatScene::rewardCardCancelButtonBounds() const {
    const Rectangle panel = rewardCardChoiceModalBounds();
    return Rectangle{panel.x + panel.width * 0.5f - 150.f, panel.y + panel.height - 66.f, 300.f, 48.f};
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
