#pragma once

#include "cards/CardInstanceFactory.hpp"
#include "cards/DrawSystem.hpp"
#include "combat/BlockSystem.hpp"
#include "combat/CardPlaySystem.hpp"
#include "combat/CardPlayValidator.hpp"
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
#include "localization/LocalizationManager.hpp"
#include "preview/CardPreviewSystem.hpp"
#include "ui/CardViewModelBuilder.hpp"
#include "ui/CombatView.hpp"
#include "ui/CombatViewModelBuilder.hpp"
#include "ui/UiFont.hpp"

#include <raylib.h>

#include <filesystem>
#include <optional>

class DebugCombatScene {
public:
    DebugCombatScene(
        const ContentRegistry& content,
        const LocalizationManager& localization,
        Random& random,
        const std::filesystem::path& assetsPath
    );

    void update(float deltaSeconds);
    void render() const;

private:
    void initializeCombat();
    void rebuildViewModel(std::optional<EntityId> previewTarget);
    void handleClick(Vector2 mousePosition);
    void playSelectedCardOn(EntityId target);
    void endPlayerTurn();

private:
    const ContentRegistry& content_;
    const LocalizationManager& localization_;
    Random& random_;

    UiFont uiFont_;

    ModifierSystem modifierSystem_;
    EffectResolver effectResolver_;
    Targeting targeting_;
    DamageSystem damageSystem_;
    BlockSystem blockSystem_;
    EnergySystem energySystem_;
    DrawSystem drawSystem_;
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

    CombatState state_;
    EntityIdGenerator entityIds_;
    CardInstanceFactory cardFactory_;

    EntityId playerId_;
    std::optional<CardInstanceId> selectedCardId_;

    CombatView view_;

    bool viewModelDirty_ = true;
    std::optional<EntityId> lastPreviewTarget_;
};
