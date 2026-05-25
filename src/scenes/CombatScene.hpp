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
#include "core/Random.hpp"
#include "data/ContentRegistry.hpp"
#include "game/GameEventBus.hpp"
#include "relics/RelicSystem.hpp"
#include "localization/LocalizationManager.hpp"
#include "preview/CardPreviewSystem.hpp"
#include "run/RunState.hpp"
#include "statuses/StatusSystem.hpp"
#include "scenes/Scene.hpp"
#include "inspect/InspectModelBuilder.hpp"
#include "ui/CardViewModelBuilder.hpp"
#include "ui/CardViewModel.hpp"
#include "ui/EnemyViewModel.hpp"
#include "ui/InspectPanelView.hpp"
#include "ui/RelicInspectModal.hpp"
#include "ui/CombatView.hpp"
#include "ui/CombatViewModelBuilder.hpp"
#include "ui/RelicViewModel.hpp"
#include "ui/UiFont.hpp"

#include <raylib.h>

#include <filesystem>
#include <functional>
#include <optional>
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

private:
    void initializeCombat();
    void rebuildViewModel(std::optional<EntityId> previewTarget);
    std::vector<RelicViewModel> buildRelicViewModels() const;
    void updateInspectInput(Vector2 mousePosition);
    void renderInspectOverlay() const;
    std::optional<EnemyViewModel> hoveredEnemyViewModel() const;
    std::optional<CardViewModel> inspectedCardViewModel() const;
    void handleMousePressed(Vector2 mousePosition);
    void handleMouseReleased(Vector2 mousePosition);
    void playSelectedCardOn(EntityId target);
    void endPlayerTurn();

    bool selectedCardCanTargetEnemy() const;
    bool selectedCardCanTargetPlayer() const;
    bool cardCanTargetEnemy(CardInstanceId cardInstanceId) const;
    bool cardCanTargetPlayer(CardInstanceId cardInstanceId) const;

    void finishCombatIfNeeded();

private:
    const ContentRegistry& content_;
    const LocalizationManager& localization_;
    Random& random_;
    const UiFont& uiFont_;
    const RunState& runState_;
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
    std::optional<CardInstanceId> selectedCardId_;
    std::optional<CardInstanceId> draggedCardId_;

    CombatView view_;
    InspectPanelView inspectPanelView_;
    RelicInspectModal relicInspectModal_;
    std::optional<CardInstanceId> inspectedCardId_;

    bool viewModelDirty_ = true;
    bool combatFinished_ = false;
    std::optional<EntityId> lastPreviewTarget_;
};
