#include "run/SadistMasochistRules.hpp"

#include "combat/CombatLog.hpp"
#include "combat/CombatState.hpp"
#include "entities/CombatEntity.hpp"
#include "game/GameEvent.hpp"

namespace SadistMasochistRules {
namespace {
bool isSadistHpDamageToMasochist(const CombatState& state, const GameEvent& event) {
    if (event.type != GameEventType::DamageDealt ||
        event.amount <= 0 ||
        !event.source.has_value() ||
        !event.target.has_value()) {
        return false;
    }

    if (!state.hasEntity(*event.source) || !state.hasEntity(*event.target)) {
        return false;
    }

    const CombatEntity& source = state.entity(*event.source);
    const CombatEntity& target = state.entity(*event.target);
    return source.definitionId == SadistActorDefinitionId &&
           target.definitionId == MasochistActorDefinitionId;
}
} // namespace

bool appliesTo(const std::string& mechanicId) {
    return mechanicId == MechanicId;
}

void handleEvent(CombatState& state, const GameEvent& event) {
    if (!isSadistHpDamageToMasochist(state, event)) {
        return;
    }

    CombatEntity& sadist = state.entity(*event.source);
    CombatEntity& masochist = state.entity(*event.target);

    // These are one-turn combat states, not permanent scaling stacks.
    // Repeated Sadist -> Masochist hits refresh the engine instead of extending it
    // across future turns.
    sadist.statuses.set(PleasureStatusId, 1, sadist.id);
    masochist.statuses.set(PainStatusId, 1, sadist.id);

    state.log.add(CombatLogEntryType::SadistHurtsMasochist);
    state.log.add(CombatLogEntryType::MasochistPainBonus);
}
} // namespace SadistMasochistRules
