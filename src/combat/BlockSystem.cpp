#include "BlockSystem.hpp"

#include "combat/CombatState.hpp"

#include <sstream>
#include <string>

namespace {
void addEntityVariables(
    CombatLogEntry::Variables& variables,
    const CombatState& state,
    const EntityId entityId,
    const std::string& prefix
) {
    if (!state.hasEntity(entityId)) {
        return;
    }
    const CombatEntity& entity = state.entity(entityId);
    variables[prefix] = entity.definitionId.empty() ? std::to_string(entityId.value) : entity.definitionId;
    variables[prefix + "_text_id"] = entity.nameTextId.value;
}

std::string modifierTrace(const ModifiedValue& modified) {
    std::ostringstream output;
    for (const ModifierBreakdownEntry& entry : modified.breakdown) {
        if (entry.before == entry.after) {
            continue;
        }
        if (output.tellp() > 0) {
            output << "; ";
        }
        output << entry.description << " " << entry.before << "->" << entry.after;
    }
    return output.str();
}
}

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

    CombatLogEntry::Variables logVariables{
        {"raw", std::to_string(rawBlock)},
        {"modified", std::to_string(modified.modified)},
        {"amount", std::to_string(modified.modified)},
        {"card", cardId.value},
        {"modifiers", modifierTrace(modified)}
    };
    addEntityVariables(logVariables, state, source, "source");
    addEntityVariables(logVariables, state, target, "target");
    state.log.add(CombatLogEntryType::BlockGained, std::move(logVariables));

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
