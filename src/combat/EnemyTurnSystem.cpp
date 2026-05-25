#include "EnemyTurnSystem.hpp"

#include <algorithm>
#include <stdexcept>

EnemyTurnSystem::EnemyTurnSystem(
    const EnemyMoveSelector& moveSelector,
    const EffectSystem& effectSystem
)
    : moveSelector_(moveSelector),
      effectSystem_(effectSystem) {}

void EnemyTurnSystem::executeTurn(
    CombatState& state,
    const EnemyDatabase& enemyDatabase,
    Random& random
) const {
    state.phase = CombatPhase::EnemyTurn;
    state.log.add("Enemy turn started");

    for (CombatEntity& enemy : state.enemies) {
        if (!enemy.isAlive()) {
            continue;
        }

        const EnemyActionDefinition& action = actionForEnemy(
            state,
            enemyDatabase,
            enemy,
            random
        );

        EffectContext context;
        context.source = enemy.id;
        context.cardInstanceId = CardInstanceId{};
        context.cardDefinitionId = CardId("enemy_action." + action.id);
        context.diceCorruption = DiceCorruption{};
        context.random = &random;

        state.log.add("Enemy action: " + action.id);
        effectSystem_.applyEffects(state, action.effects, context);
    }

    state.log.add("Enemy turn ended");
}

const EnemyActionDefinition& EnemyTurnSystem::actionForEnemy(
    const CombatState& state,
    const EnemyDatabase& enemyDatabase,
    const CombatEntity& enemy,
    Random& random
) const {
    const EnemyDefinition& definition = enemyDatabase.get(EnemyId(enemy.definitionId));

    const std::optional<EnemyIntentState> intent = moveSelector_.intentFor(
        state,
        enemy.id
    );

    if (intent.has_value()) {
        const auto iterator = std::find_if(
            definition.actions.begin(),
            definition.actions.end(),
            [&intent](const EnemyActionDefinition& action) {
                return action.id == intent->actionId;
            }
        );

        if (iterator != definition.actions.end()) {
            return *iterator;
        }
    }

    return moveSelector_.selectAction(state, definition, enemy.id, random);
}
