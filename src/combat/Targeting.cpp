#include "Targeting.hpp"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <vector>

namespace {
EntityId chooseRandomTarget(const std::vector<EntityId>& targets, const EffectContext& context) {
    if (targets.empty()) {
        return EntityId{};
    }

    if (context.random == nullptr) {
        return targets.front();
    }

    const int index = context.random->rangeInclusive(
        0,
        static_cast<int>(targets.size()) - 1
    );
    return targets[static_cast<std::size_t>(index)];
}

bool isAliveEnemy(const CombatState& state, const EntityId id) {
    return state.hasEntity(id) && state.isEnemy(id) && state.entity(id).isAlive();
}

bool isAliveAlly(const CombatState& state, const EntityId id, const EntityId source) {
    return state.hasEntity(id) && state.isPlayer(id) && id != source && state.entity(id).isAlive();
}

std::vector<EntityId> aliveAlliesExceptSource(const CombatState& state, const EntityId source) {
    std::vector<EntityId> allies;
    for (const EntityId candidate : state.alivePlayerIds()) {
        if (candidate != source) {
            allies.push_back(candidate);
        }
    }
    return allies;
}

std::optional<EntityId> explicitEnemyTarget(const CombatState& state, const EffectContext& context) {
    if (context.explicitEnemyTarget.has_value() && isAliveEnemy(state, *context.explicitEnemyTarget)) {
        return context.explicitEnemyTarget;
    }

    if (context.explicitTarget.has_value() && isAliveEnemy(state, *context.explicitTarget)) {
        return context.explicitTarget;
    }

    return std::nullopt;
}

std::optional<EntityId> explicitAllyTarget(const CombatState& state, const EffectContext& context) {
    if (context.explicitAllyTarget.has_value() && isAliveAlly(state, *context.explicitAllyTarget, context.source)) {
        return context.explicitAllyTarget;
    }

    if (context.explicitTarget.has_value() && isAliveAlly(state, *context.explicitTarget, context.source)) {
        return context.explicitTarget;
    }

    return std::nullopt;
}
} // namespace

std::vector<EntityId> Targeting::resolveTargets(
    const CombatState& state,
    const EffectTarget target,
    const EffectContext& context
) const {
    switch (target) {
        case EffectTarget::Self:
            if (!state.hasEntity(context.source) || !state.entity(context.source).isAlive()) {
                return {};
            }
            return {context.source};

        case EffectTarget::SingleEnemy: {
            const std::optional<EntityId> targetId = explicitEnemyTarget(state, context);
            if (targetId.has_value()) {
                return {*targetId};
            }

            // A card can contain several effects or repeated hits for the same
            // explicitly selected enemy. If an earlier effect killed that enemy,
            // the remaining effects must harmlessly miss instead of aborting the
            // whole game. The absence of any explicit selection is still a caller
            // error and remains fatal.
            if (context.explicitEnemyTarget.has_value()) {
                return {};
            }

            if (context.explicitTarget.has_value() &&
                state.hasEntity(*context.explicitTarget) &&
                state.isEnemy(*context.explicitTarget)) {
                return {};
            }

            throw std::runtime_error("SingleEnemy effect requires explicit enemy target");
        }

        case EffectTarget::AllEnemies:
            return state.aliveEnemyIds();

        case EffectTarget::RandomEnemy: {
            const std::vector<EntityId> enemies = state.aliveEnemyIds();
            if (enemies.empty()) {
                return {};
            }
            return {chooseRandomTarget(enemies, context)};
        }

        case EffectTarget::Ally: {
            const std::optional<EntityId> targetId = explicitAllyTarget(state, context);
            if (targetId.has_value()) {
                return {*targetId};
            }

            // Mixed cards such as Sadist -> Masochist + enemy strike choose the
            // enemy as the primary UI target. In that case the ally side is
            // unambiguous for the current two-actor party: the other living actor.
            return aliveAlliesExceptSource(state, context.source);
        }

        case EffectTarget::AllAllies:
            return state.alivePlayerIds();

        case EffectTarget::RandomAlly: {
            const std::vector<EntityId> allies = aliveAlliesExceptSource(state, context.source);
            if (allies.empty()) {
                return {};
            }
            return {chooseRandomTarget(allies, context)};
        }
    }

    throw std::runtime_error("Unknown effect target");
}
