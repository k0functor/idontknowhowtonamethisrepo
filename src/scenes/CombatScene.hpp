#pragma once

#include "cards/CardInstanceFactory.hpp"
#include "cards/DrawSystem.hpp"
#include "combat/BlockSystem.hpp"
#include "combat/CardPlaySystem.hpp"
#include "combat/CardPlayValidator.hpp"
#include "combat/CombatController.hpp"
#include "combat/CombatOutcome.hpp"
#include "combat/CombatResult.hpp"
#include "combat/CombatState.hpp"
#include "combat/DamageSystem.hpp"
#include "combat/EffectResolver.hpp"
#include "combat/EffectSystem.hpp"
#include "combat/EnergySystem.hpp"
#include "combat/EnemyMoveSelector.hpp"
#include "combat/EnemyTurnSystem.hpp"
#include "combat/ModifierSystem.hpp"
#include "combat/PlayerTurnSystem.hpp"
#include "combat/Targeting.hpp"
#include "combat/TurnSystem.hpp"
#include "consumables/ConsumableSystem.hpp"
#include "core/Random.hpp"
#include "data/ContentRegistry.hpp"
#include "drones/DroneSystem.hpp"
#include "game/GameEventBus.hpp"
#include "inspect/InspectModelBuilder.hpp"
#include "localization/LocalizationManager.hpp"
#include "preview/CardPreviewSystem.hpp"
#include "relics/RelicSystem.hpp"
#include "rewards/RewardSelection.hpp"
#include "rewards/RewardState.hpp"
#include "run/RunState.hpp"
#include "scenes/Scene.hpp"
#include "statuses/StatusSystem.hpp"
#include "ui/CardViewModel.hpp"
#include "ui/CardViewModelBuilder.hpp"
#include "ui/CombatView.hpp"
#include "ui/CombatViewModelBuilder.hpp"
#include "ui/EnemyViewModel.hpp"
#include "ui/InspectPanelView.hpp"
#include "ui/RelicInspectModal.hpp"
#include "ui/RelicViewModel.hpp"
#include "ui/UiFont.hpp"

#include <raylib.h>

#include <cstddef>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

class CombatScene final : public Scene {
public:
    CombatScene(
        const ContentRegistry& content,
        const LocalizationManager& localization,
        Random& random,
        const UiFont& uiFont,
        const RunState& runState,
        std::function<void(const CombatResult&)> onCombatWon,
        std::function<void(const CombatResult&)> onCombatLost
    );

    void update(float deltaSeconds) override;
    void render() const override;
    void onLocalizationChanged() override;
    bool handleDebugCommand(const std::vector<std::string>& tokens, std::string& output) override;

private:
    enum class PileOverlayMode {
        None,
        DrawPile,
        DiscardPile,
        ExhaustPile
    };

    void initializeCombat();
    void rebuildViewModel(std::optional<EntityId> previewTarget);
    EntityId primaryPlayerId() const;
    EntityId sourceForCard(const CardInstance& card) const;
    EntityId sourceForCard(CardInstanceId cardInstanceId) const;
    bool isSadistMasochistParty() const;
    bool isDroneCyborgParty() const;
    std::vector<RelicViewModel> buildRelicViewModels() const;
    std::vector<DroneSlotViewModel> buildDroneSlotViewModels() const;

    void updateInspectInput(Vector2 mousePosition);
    void renderInspectOverlay() const;
    std::optional<EnemyViewModel> hoveredEnemyViewModel() const;
    std::optional<PlayerViewModel> hoveredPlayerViewModel() const;
    std::optional<CardViewModel> inspectedCardViewModel() const;

    void handleKeyboardCombatInput();
    void selectCardByOffset(int offset);
    void selectCard(CardInstanceId cardInstanceId);
    void cycleKeyboardTarget(int offset);
    void ensureKeyboardTargetForSelectedCard();
    void clearCardSelection();

    void handleMousePressed(Vector2 mousePosition);
    void openConsumableConfirmation(std::size_t index);
    void cancelConsumableConfirmation();
    void confirmConsumableUse();
    void tryUseConsumable(std::size_t index);
    void updateConsumableConfirmationInput(Vector2 mousePosition);
    void renderConsumableConfirmationModal() const;

    bool combatItemInspectOpen() const;
    void openRelicInspect(std::size_t index);
    void openConsumableInspect(std::size_t index);
    void closeCombatItemInspect();
    void updateCombatItemInspectInput(Vector2 mousePosition);
    void renderCombatItemInspectModal() const;
    Rectangle combatItemInspectModalBounds() const;
    Rectangle combatItemInspectCloseButtonBounds(Rectangle modal) const;
    Rectangle combatItemInspectPreviousButtonBounds(Rectangle modal) const;
    Rectangle combatItemInspectNextButtonBounds(Rectangle modal) const;
    void inspectPreviousItem();
    void inspectNextItem();
    std::optional<std::size_t> nextFilledConsumableIndex(std::size_t start, int direction) const;

    Rectangle consumableConfirmationBounds() const;
    Rectangle consumableConfirmButtonBounds(Rectangle modal) const;
    Rectangle consumableCancelButtonBounds(Rectangle modal) const;
    void handleMouseReleased(Vector2 mousePosition);
    void playSelectedCardOn(EntityId target);
    void endPlayerTurn();

    std::vector<CardInstanceId> handCardIds() const;
    std::optional<std::size_t> handCardIndex(CardInstanceId cardInstanceId) const;
    std::vector<EntityId> targetCandidatesForCard(CardInstanceId cardInstanceId) const;
    std::optional<EntityId> preferredTargetForCard(CardInstanceId cardInstanceId) const;
    std::optional<EntityId> previewTargetForSelectedCard() const;
    std::optional<EntityId> arrowTargetForCard(CardInstanceId cardInstanceId) const;
    void renderTargetingArrow() const;

    Rectangle drawPileButtonBounds() const;
    Rectangle discardPileButtonBounds() const;
    Rectangle exhaustPileButtonBounds() const;
    Rectangle energyBubbleBounds() const;
    Rectangle pileOverlayBounds() const;
    Rectangle pileOverlayGridBounds(Rectangle modal) const;
    Rectangle pileOverlayCloseButtonBounds(Rectangle modal) const;
    Rectangle pileOverlayCardBounds(Rectangle grid, std::size_t index, float scrollOffset) const;
    float pileOverlayMaxScroll(Rectangle grid, std::size_t count) const;
    const std::vector<CardInstance>& activePileCards() const;
    std::string activePileTitle() const;
    void openPileOverlay(PileOverlayMode mode);
    void closePileOverlay();
    void updatePileOverlay(Vector2 mousePosition);
    void renderEnergyBubble() const;
    void renderPileButtons() const;
    void renderPileOverlay() const;
    void renderPileCard(const CardInstance& card, Rectangle bounds) const;
    std::optional<std::size_t> hoveredPileCardIndex(Vector2 mousePosition) const;
    CardViewModel cardViewModelForInstance(const CardInstance& card) const;
    void renderPileCardInspectPanel() const;

    bool selectedCardCanTargetEnemy() const;
    bool selectedCardCanTargetPlayer() const;
    bool cardCanTargetEnemy(CardInstanceId cardInstanceId) const;
    bool cardCanTargetPlayer(CardInstanceId cardInstanceId) const;

    void finishCombatIfNeeded();

    void openRewardModalIfNeeded();
    void updateRewardModalInput(Vector2 mousePosition);
    void renderRewardModal() const;
    void renderRewardRelicInspect(const RewardOption& option, Rectangle row) const;
    void renderDefeatModal() const;

    Rectangle rewardModalBounds() const;
    Rectangle rewardOptionRowBounds(std::size_t index) const;
    Rectangle rewardContinueButtonBounds() const;

    Rectangle rewardCardChoiceModalBounds() const;
    Rectangle rewardCardChoiceOptionBounds(std::size_t index) const;
    Rectangle rewardCardChoiceCancelBounds() const;
    Rectangle rewardCardChoiceConfirmBounds() const;

    void openRewardCardChoice(std::size_t optionIndex);
    void closeRewardCardChoice();
    void confirmRewardCardChoice();
    void takeRewardOption(std::size_t optionIndex);

    void updateRewardCardChoiceInput(Vector2 mousePosition);
    void renderRewardCardChoiceModal() const;

    const RewardOption* activeRewardOption() const;
    RewardOption* activeRewardOption();

    std::string rewardOptionTitle(const RewardOption& option) const;
    std::string rewardOptionDescription(const RewardOption& option) const;
    std::string rewardCardName(const CardId& cardId) const;
    std::string rewardCardDescription(const CardId& cardId) const;
    std::string rewardRelicName(const std::string& relicId) const;
    std::string rewardRelicDescription(const std::string& relicId) const;
    std::string localizedOrFallback(const TextId& textId, const std::string& fallback) const;

private:
    const ContentRegistry& content_;
    const LocalizationManager& localization_;
    Random& random_;
    const UiFont& uiFont_;
    const RunState& runState_;

    std::function<RewardState(const CombatResult&)> createRewardOnVictory_;
    std::function<void(const RewardState&, const RewardSelection&)> onRewardAccepted_;
    std::function<void(const CombatResult&)> onCombatWon_;
    std::function<void(const CombatResult&)> onCombatLost_;

    GameEventBus eventBus_;
    ModifierSystem modifierSystem_;
    CombatController combatController_;
    RelicSystem relicSystem_;
    EffectResolver effectResolver_;
    Targeting targeting_;
    DamageSystem damageSystem_;
    BlockSystem blockSystem_;
    EnergySystem energySystem_;
    DrawSystem drawSystem_;
    StatusSystem statusSystem_;
    ConsumableSystem consumableSystem_;
    DroneSystem droneSystem_;
    CardPlayValidator validator_;
    EffectSystem effectSystem_;
    CardPlaySystem cardPlaySystem_;
    CardPreviewSystem previewSystem_;

    EnemyMoveSelector enemyMoveSelector_;
    PlayerTurnSystem playerTurnSystem_;
    EnemyTurnSystem enemyTurnSystem_;
    TurnSystem turnSystem_;

    CardViewModelBuilder cardViewModelBuilder_;
    CombatViewModelBuilder combatViewModelBuilder_;
    InspectModelBuilder inspectModelBuilder_;

    CombatState state_;
    CombatResult finalResult_;
    EntityIdGenerator entityIds_;
    CardInstanceFactory cardFactory_;

    EntityId playerId_;
    std::vector<std::string> combatConsumableIds_;
    std::optional<CardInstanceId> selectedCardId_;
    std::optional<CardInstanceId> draggedCardId_;
    std::optional<EntityId> keyboardTargetId_;

    CombatView view_;
    InspectPanelView inspectPanelView_;
    RelicInspectModal relicInspectModal_;
    std::optional<CardInstanceId> inspectedCardId_;
    std::optional<std::size_t> inspectedPileCardIndex_;
    std::optional<std::size_t> inspectedRelicIndex_;
    std::optional<std::size_t> inspectedConsumableIndex_;
    std::optional<std::size_t> pendingConsumableIndex_;
    PileOverlayMode pileOverlayMode_ = PileOverlayMode::None;
    float pileOverlayScrollOffset_ = 0.f;

    std::optional<RewardState> reward_;
    RewardSelection rewardSelection_;
    std::optional<std::size_t> activeRewardOptionIndex_;
    std::optional<std::size_t> selectedRewardCardIndex_;
    bool rewardCardChoiceOpen_ = false;
    bool rewardAccepted_ = false;

    bool viewModelDirty_ = true;
    bool combatFinished_ = false;
    std::optional<EntityId> lastPreviewTarget_;
};
