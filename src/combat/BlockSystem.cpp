#include "BlockSystem.hpp"

#include "combat/CombatState.hpp"

#include <string>

BlockSystem::BlockSystem(const ModifierSystem& modifierSystem)
    : modifierSystem_(modifierSystem) {}

BlockResult BlockSystem::gainBlock(
    CombatState& state,
    const EntityId source,
    const EntityId target,
    const int rawBlock,
    const CardId& cardId,
    const DiceCorruption diceCorruption
) const {
    ModifierContext modifierContext;
    modifierContext.effectType = EffectType::Block;
    modifierContext.source = source;
    modifierContext.target = target;
    modifierContext.cardId = cardId;
    modifierContext.diceCorruption = diceCorruption;

    const ModifiedValue modified = modifierSystem_.modifyValue(
        state,
        rawBlock,
        modifierContext
    );

    CombatEntity& targetEntity = state.entity(target);
    targetEntity.block += modified.modified;

    state.log.add(
        "Block: raw=" + std::to_string(rawBlock) +
        ", modified=" + std::to_string(modified.modified)
    );

    return BlockResult{rawBlock, modified.modified};
}

ModifiedValueRange BlockSystem::previewBlock(
    const CombatState& state,
    const EntityId source,
    const EntityId target,
    const int rawMin,
    const int rawMax,
    const CardId& cardId,
    const DiceCorruption diceCorruption
) const {
    ModifierContext modifierContext;
    modifierContext.effectType = EffectType::Block;
    modifierContext.source = source;
    modifierContext.target = target;
    modifierContext.cardId = cardId;
    modifierContext.diceCorruption = diceCorruption;
    modifierContext.preview = true;

    return modifierSystem_.modifyRange(state, rawMin, rawMax, modifierContext);
}
