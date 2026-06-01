#include "ConsumableSystem.hpp"

#include "combat/EffectContext.hpp"

ConsumableSystem::ConsumableSystem(const ConsumableDatabase& database)
    : database_(database) {}

bool ConsumableSystem::useConsumable(
    CombatState& state,
    const std::string& consumableId,
    const EntityId source,
    const std::optional<EntityId> explicitTarget,
    const EffectSystem& effectSystem,
    Random& random
) const {
    const ConsumableId id(consumableId);
    if (!database_.contains(id) || !state.hasEntity(source)) {
        return false;
    }

    const ConsumableDefinition& definition = database_.get(id);

    EffectContext context;
    context.source = source;
    context.explicitTarget = explicitTarget.has_value() ? explicitTarget : std::optional<EntityId>{source};
    context.cardDefinitionId = CardId("consumable." + consumableId);
    context.random = &random;

    effectSystem.applyEffects(state, definition.effects, context);
    state.log.add(CombatLogEntryType::UsedConsumable, {{"consumable", consumableId}});
    return true;
}
