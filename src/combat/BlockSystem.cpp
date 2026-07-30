#include "BlockSystem.hpp"

#include "combat/CombatState.hpp"

#include <string>

BlockSystem::BlockSystem(const ModifierSystem& modifierSystem, const GameEventBus* eventBus)
    : modifierSystem_(modifierSystem),
      eventBus_(eventBus) {}

BlockResult BlockSystem::gainBlock(
    CombatState& state,
    const EntityId source,
    const EntityId target,
    const int rawBlock,
    const CardId& cardId,
    const DiceCorruption diceCorruption,
    const bool usesActorStats
) const {
    ModifierContext modifierContext;
    modifierContext.effectType = EffectType::Block;
    modifierContext.source = source;
    modifierContext.target = target;
    modifierContext.cardId = cardId;
    modifierContext.diceCorruption = diceCorruption;
    modifierContext.usesActorStats = usesActorStats;

    const ModifiedValue modified = modifierSystem_.modifyValue(
        state,
        rawBlock,
        modifierContext
    );

    CombatEntity& targetEntity = state.entity(target);
    targetEntity.block += modified.modified;
    if (state.isPlayer(target)) {
        state.telemetry.blockGainedByPlayers += modified.modified;
    }

    state.log.add(
        CombatLogEntryType::BlockGained,
        {
            {"raw", std::to_string(rawBlock)},
            {"modified", std::to_string(modified.modified)}
        }
    );

    if (eventBus_ != nullptr) {
        GameEvent event;
        event.type = GameEventType::BlockGained;
        event.source = source;
        event.target = target;
        event.cardDefinitionId = cardId;
        event.effectType = EffectType::Block;
        event.amount = modified.modified;
        event.turn = state.turn;
        eventBus_->emit(event);
    }

    return BlockResult{rawBlock, modified.modified};
}

ModifiedValueRange BlockSystem::previewBlock(
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
    modifierContext.effectType = EffectType::Block;
    modifierContext.source = source;
    modifierContext.target = target;
    modifierContext.cardId = cardId;
    modifierContext.diceCorruption = diceCorruption;
    modifierContext.usesActorStats = usesActorStats;
    modifierContext.preview = true;

    return modifierSystem_.modifyRange(state, rawMin, rawMax, modifierContext);
}
