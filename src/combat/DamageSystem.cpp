#include "DamageSystem.hpp"

#include "combat/CombatState.hpp"

#include <algorithm>
#include <string>

DamageSystem::DamageSystem(const ModifierSystem& modifierSystem, const GameEventBus* eventBus)
    : modifierSystem_(modifierSystem),
      eventBus_(eventBus) {}

DamageResult DamageSystem::dealDamage(
    CombatState& state,
    const EntityId source,
    const EntityId target,
    const int rawDamage,
    const CardId& cardId,
    const DiceCorruption diceCorruption,
    const bool usesActorStats
) const {
    ModifierContext modifierContext;
    modifierContext.effectType = EffectType::Damage;
    modifierContext.source = source;
    modifierContext.target = target;
    modifierContext.hasTarget = true;
    modifierContext.cardId = cardId;
    modifierContext.diceCorruption = diceCorruption;
    modifierContext.usesActorStats = usesActorStats;

    const ModifiedValue modified = modifierSystem_.modifyValue(
        state,
        rawDamage,
        modifierContext
    );

    CombatEntity& targetEntity = state.entity(target);

    DamageResult result;
    result.rawDamage = rawDamage;
    result.modifiedDamage = modified.modified;
    result.blockedDamage = std::min(targetEntity.block, result.modifiedDamage);

    targetEntity.block -= result.blockedDamage;

    result.hpDamage = std::max(0, result.modifiedDamage - result.blockedDamage);
    targetEntity.health.takeDamage(result.hpDamage);
    result.killed = targetEntity.health.isDead();

    if (state.isEnemy(target) && state.isPlayer(source)) {
        state.telemetry.damageDealtToEnemies += result.hpDamage;
        state.telemetry.maximumSingleHit = std::max(state.telemetry.maximumSingleHit, result.hpDamage);
    } else if (state.isPlayer(target)) {
        state.telemetry.damageTakenByPlayers += result.hpDamage;
        state.telemetry.damageBlockedByPlayers += result.blockedDamage;
    }

    state.log.add(
        CombatLogEntryType::DamageDealt,
        {
            {"raw", std::to_string(result.rawDamage)},
            {"modified", std::to_string(result.modifiedDamage)},
            {"blocked", std::to_string(result.blockedDamage)},
            {"hp", std::to_string(result.hpDamage)}
        }
    );

    if (eventBus_ != nullptr) {
        GameEvent dealt;
        dealt.type = GameEventType::DamageDealt;
        dealt.source = source;
        dealt.target = target;
        dealt.cardDefinitionId = cardId;
        dealt.effectType = EffectType::Damage;
        dealt.amount = result.hpDamage;
        dealt.blockedAmount = result.blockedDamage;
        dealt.turn = state.turn;
        eventBus_->emit(dealt);

        GameEvent taken = dealt;
        taken.type = GameEventType::DamageTaken;
        eventBus_->emit(taken);

        if (result.killed && state.isEnemy(target)) {
            GameEvent killed;
            killed.type = GameEventType::EnemyKilled;
            killed.source = source;
            killed.target = target;
            killed.cardDefinitionId = cardId;
            killed.amount = result.hpDamage;
            killed.turn = state.turn;
            eventBus_->emit(killed);
        }
    }

    return result;
}

DamagePreview DamageSystem::previewDamage(
    const CombatState& state,
    const EntityId source,
    const EntityId target,
    const int rawMin,
    const int rawMax,
    const CardId& cardId,
    const DiceCorruption diceCorruption,
    const bool usesActorStats
) const {
    ModifierContext modifierContext;
    modifierContext.effectType = EffectType::Damage;
    modifierContext.source = source;
    modifierContext.target = target;
    modifierContext.hasTarget = true;
    modifierContext.cardId = cardId;
    modifierContext.diceCorruption = diceCorruption;
    modifierContext.usesActorStats = usesActorStats;
    modifierContext.preview = true;

    const ModifiedValueRange modified = modifierSystem_.modifyRange(
        state,
        rawMin,
        rawMax,
        modifierContext
    );

    const CombatEntity& targetEntity = state.entity(target);

    DamagePreview preview;
    preview.rawMin = rawMin;
    preview.rawMax = rawMax;
    preview.modifiedMin = modified.modifiedMin;
    preview.modifiedMax = modified.modifiedMax;
    preview.blockedMin = std::min(targetEntity.block, preview.modifiedMin);
    preview.blockedMax = std::min(targetEntity.block, preview.modifiedMax);
    preview.hpDamageMin = std::max(0, preview.modifiedMin - preview.blockedMin);
    preview.hpDamageMax = std::max(0, preview.modifiedMax - preview.blockedMax);
    return preview;
}

DamagePreview DamageSystem::previewOutgoingDamage(
    const CombatState& state,
    const EntityId source,
    const int rawMin,
    const int rawMax,
    const CardId& cardId,
    const DiceCorruption diceCorruption,
    const bool usesActorStats
) const {
    ModifierContext modifierContext;
    modifierContext.effectType = EffectType::Damage;
    modifierContext.source = source;
    modifierContext.target = source;
    modifierContext.hasTarget = false;
    modifierContext.cardId = cardId;
    modifierContext.diceCorruption = diceCorruption;
    modifierContext.usesActorStats = usesActorStats;
    modifierContext.preview = true;

    const ModifiedValueRange modified = modifierSystem_.modifyRange(
        state,
        rawMin,
        rawMax,
        modifierContext
    );

    DamagePreview preview;
    preview.rawMin = rawMin;
    preview.rawMax = rawMax;
    preview.modifiedMin = modified.modifiedMin;
    preview.modifiedMax = modified.modifiedMax;
    preview.blockedMin = 0;
    preview.blockedMax = 0;
    preview.hpDamageMin = preview.modifiedMin;
    preview.hpDamageMax = preview.modifiedMax;
    return preview;
}
