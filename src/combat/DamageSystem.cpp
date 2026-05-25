#include "DamageSystem.hpp"

#include "combat/CombatState.hpp"

#include <algorithm>
#include <string>

DamageSystem::DamageSystem(const ModifierSystem& modifierSystem)
    : modifierSystem_(modifierSystem) {}

DamageResult DamageSystem::dealDamage(
    CombatState& state,
    const EntityId source,
    const EntityId target,
    const int rawDamage,
    const CardId& cardId,
    const DiceCorruption diceCorruption
) const {
    ModifierContext modifierContext;
    modifierContext.effectType = EffectType::Damage;
    modifierContext.source = source;
    modifierContext.target = target;
    modifierContext.hasTarget = true;
    modifierContext.cardId = cardId;
    modifierContext.diceCorruption = diceCorruption;

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

    state.log.add(
        "Damage: raw=" + std::to_string(result.rawDamage) +
        ", modified=" + std::to_string(result.modifiedDamage) +
        ", blocked=" + std::to_string(result.blockedDamage) +
        ", hp=" + std::to_string(result.hpDamage)
    );

    return result;
}

DamagePreview DamageSystem::previewDamage(
    const CombatState& state,
    const EntityId source,
    const EntityId target,
    const int rawMin,
    const int rawMax,
    const CardId& cardId,
    const DiceCorruption diceCorruption
) const {
    ModifierContext modifierContext;
    modifierContext.effectType = EffectType::Damage;
    modifierContext.source = source;
    modifierContext.target = target;
    modifierContext.hasTarget = true;
    modifierContext.cardId = cardId;
    modifierContext.diceCorruption = diceCorruption;
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
    const DiceCorruption diceCorruption
) const {
    ModifierContext modifierContext;
    modifierContext.effectType = EffectType::Damage;
    modifierContext.source = source;
    modifierContext.target = source;
    modifierContext.hasTarget = false;
    modifierContext.cardId = cardId;
    modifierContext.diceCorruption = diceCorruption;
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
