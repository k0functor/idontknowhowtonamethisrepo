#pragma once

#include "cards/CardId.hpp"
#include "combat/ModifierSystem.hpp"
#include "dice/DiceCorruption.hpp"
#include "entities/EntityId.hpp"
#include "game/GameEventBus.hpp"

class CombatState;

struct BlockResult {
    int rawBlock = 0;
    int modifiedBlock = 0;
};

class BlockSystem {
public:
    explicit BlockSystem(const ModifierSystem& modifierSystem, const GameEventBus* eventBus = nullptr);

    BlockResult gainBlock(
        CombatState& state,
        EntityId source,
        EntityId target,
        int rawBlock,
        const CardId& cardId,
        DiceCorruption diceCorruption
    ) const;

    ModifiedValueRange previewBlock(
        const CombatState& state,
        EntityId source,
        EntityId target,
        int rawMin,
        int rawMax,
        const CardId& cardId,
        DiceCorruption diceCorruption
    ) const;

private:
    const ModifierSystem& modifierSystem_;
    const GameEventBus* eventBus_ = nullptr;
};
