#pragma once

#include "cards/CardId.hpp"
#include "combat/DamagePreview.hpp"
#include "combat/ModifierSystem.hpp"
#include "dice/DiceCorruption.hpp"
#include "entities/EntityId.hpp"
#include "game/GameEventBus.hpp"

class CombatState;

struct DamageResult {
    int rawDamage = 0;
    int modifiedDamage = 0;
    int blockedDamage = 0;
    int hpDamage = 0;
    bool killed = false;
};

class DamageSystem {
public:
    explicit DamageSystem(const ModifierSystem& modifierSystem, const GameEventBus* eventBus = nullptr);

    DamageResult dealDamage(
        CombatState& state,
        EntityId source,
        EntityId target,
        int rawDamage,
        const CardId& cardId,
        DiceCorruption diceCorruption,
        bool usesActorStats
    ) const;

    DamagePreview previewDamage(
        const CombatState& state,
        EntityId source,
        EntityId target,
        int rawMin,
        int rawMax,
        const CardId& cardId,
        DiceCorruption diceCorruption,
        bool usesActorStats
    ) const;

    DamagePreview previewOutgoingDamage(
        const CombatState& state,
        EntityId source,
        int rawMin,
        int rawMax,
        const CardId& cardId,
        DiceCorruption diceCorruption,
        bool usesActorStats
    ) const;

private:
    const ModifierSystem& modifierSystem_;
    const GameEventBus* eventBus_ = nullptr;
};
