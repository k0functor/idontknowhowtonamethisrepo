#include "Targeting.hpp"

#include <stdexcept>

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
            return {enemies.front()};
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
            return {allies.front()};
        }
    }

    throw std::runtime_error("Unknown effect target");
}
