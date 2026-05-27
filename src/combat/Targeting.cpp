#include "Targeting.hpp"

#include <cstddef>
#include <stdexcept>

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
}

std::vector<EntityId> Targeting::resolveTargets(
    const CombatState& state,
    const EffectTarget target,
    const EffectContext& context
) const {
    switch (target) {
        case EffectTarget::Self:
            if (context.explicitTarget.has_value() && state.hasEntity(*context.explicitTarget) && state.isPlayer(*context.explicitTarget)) {
                return {*context.explicitTarget};
            }
            return {context.source};

        case EffectTarget::SingleEnemy:
            if (!context.explicitTarget.has_value()) {
                throw std::runtime_error("SingleEnemy effect requires explicit target");
            }
            return {*context.explicitTarget};

        case EffectTarget::AllEnemies:
            return state.aliveEnemyIds();

        case EffectTarget::RandomEnemy: {
            const std::vector<EntityId> enemies = state.aliveEnemyIds();
            if (enemies.empty()) {
                return {};
            }
            return {chooseRandomTarget(enemies, context)};
        }

        case EffectTarget::Ally:
            if (!context.explicitTarget.has_value()) {
                throw std::runtime_error("Ally effect requires explicit target");
            }
            return {*context.explicitTarget};

        case EffectTarget::AllAllies:
            return state.alivePlayerIds();

        case EffectTarget::RandomAlly: {
            const std::vector<EntityId> allies = state.alivePlayerIds();
            if (allies.empty()) {
                return {};
            }
            return {chooseRandomTarget(allies, context)};
        }
    }

    throw std::runtime_error("Unknown effect target");
}
