#include "BossPhaseSystem.hpp"

#include "combat/BossPhaseRules.hpp"
#include "enemies/EnemyInstance.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace {
EnemyAiState& mutableAiStateFor(CombatState& state, const EntityId enemyId) {
    const auto iterator = std::find_if(
        state.enemyAiStates.begin(),
        state.enemyAiStates.end(),
        [enemyId](const EnemyAiState& value) { return value.enemyId == enemyId; }
    );
    if (iterator != state.enemyAiStates.end()) {
        return *iterator;
    }

    EnemyAiState value;
    value.enemyId = enemyId;
    state.enemyAiStates.push_back(std::move(value));
    return state.enemyAiStates.back();
}

void scaleSummonedEnemy(CombatEntity& enemy, const float multiplier) {
    if (multiplier == 1.f) {
        return;
    }
    const int maximum = std::max(
        1,
        static_cast<int>(static_cast<float>(enemy.health.maximum()) * multiplier + 0.5f)
    );
    enemy.health.setMaximum(maximum);
    enemy.health.setCurrent(maximum);
}

void applyEffects(
    const EffectSystem& effectSystem,
    CombatState& state,
    const EntityId source,
    const std::vector<EffectDefinition>& effects,
    Random& random,
    const std::string& phaseId
) {
    if (effects.empty() || !state.hasEntity(source) || !state.entity(source).isAlive()) {
        return;
    }

    EffectContext context;
    context.source = source;
    context.cardDefinitionId = CardId("enemy_phase." + phaseId);
    context.random = &random;
    effectSystem.applyEffects(state, effects, context);
}

void summonPhaseEnemies(
    CombatState& state,
    const EnemyDatabase& enemyDatabase,
    const EntityId bossId,
    const EnemyPhaseDefinition& phase
) {
    for (const std::string& summonId : phase.summonEnemyIds) {
        if (state.aliveEnemyCount() >= static_cast<std::size_t>(phase.maximumAliveEnemies)) {
            break;
        }
        if (!enemyDatabase.contains(EnemyId(summonId))) {
            continue;
        }

        const EnemyDefinition& summonDefinition = enemyDatabase.get(EnemyId(summonId));
        CombatEntity summoned = makeEnemyEntity(summonDefinition, state.createDynamicEntityId());
        scaleSummonedEnemy(summoned, state.enemyHpMultiplier);
        const EntityId summonedId = summoned.id;
        state.enemies.push_back(std::move(summoned));
        state.log.add(
            CombatLogEntryType::EnemySummoned,
            {
                {"boss", state.entity(bossId).definitionId},
                {"enemy", summonId},
                {"enemy_text_id", summonDefinition.nameTextId.value},
                {"entity_id", std::to_string(summonedId.value)}
            }
        );
    }
}
}

BossPhaseSystem::BossPhaseSystem(
    const EnemyDatabase& enemyDatabase,
    const EffectSystem& effectSystem
)
    : enemyDatabase_(enemyDatabase),
      effectSystem_(effectSystem) {}

bool BossPhaseSystem::synchronizePhases(CombatState& state, Random& random) const {
    std::vector<EntityId> bossIds;
    bossIds.reserve(state.enemies.size());
    for (const CombatEntity& enemy : state.enemies) {
        if (!enemy.isAlive() || !enemyDatabase_.contains(EnemyId(enemy.definitionId))) {
            continue;
        }
        const EnemyDefinition& definition = enemyDatabase_.get(EnemyId(enemy.definitionId));
        if (!definition.phases.empty()) {
            bossIds.push_back(enemy.id);
        }
    }

    bool changed = false;
    for (const EntityId bossId : bossIds) {
        if (!state.hasEntity(bossId) || !state.entity(bossId).isAlive()) {
            continue;
        }

        const CombatEntity& boss = state.entity(bossId);
        const EnemyDefinition& definition = enemyDatabase_.get(EnemyId(boss.definitionId));
        EnemyAiState& aiState = mutableAiStateFor(state, bossId);
        const int desiredIndex = BossPhaseRules::phaseIndexForHp(
            definition,
            boss,
            aiState.activePhaseIndex
        );

        for (int index = aiState.activePhaseIndex + 1; index <= desiredIndex; ++index) {
            if (index < 0 || index >= static_cast<int>(definition.phases.size())) {
                continue;
            }

            const EnemyPhaseDefinition& phase = definition.phases[static_cast<std::size_t>(index)];
            aiState.activePhaseIndex = index;
            aiState.activePhaseId = phase.id;
            aiState.lastActionId.clear();
            aiState.consecutiveUses = 0;
            aiState.cooldowns.clear();
            aiState.lastArenaEffectTurn = 0;

            state.log.add(
                CombatLogEntryType::BossPhaseChanged,
                {
                    {"boss", definition.id.value},
                    {"boss_text_id", definition.nameTextId.value},
                    {"phase", phase.id},
                    {"phase_text_id", phase.nameTextId.value}
                }
            );

            applyEffects(effectSystem_, state, bossId, phase.onEnterEffects, random, phase.id);
            summonPhaseEnemies(state, enemyDatabase_, bossId, phase);
            changed = true;
        }
    }

    return changed;
}

void BossPhaseSystem::applyPlayerTurnEffects(CombatState& state, Random& random) const {
    std::vector<EntityId> bossIds;
    for (const CombatEntity& enemy : state.enemies) {
        if (enemy.isAlive() && enemyDatabase_.contains(EnemyId(enemy.definitionId))) {
            const EnemyDefinition& definition = enemyDatabase_.get(EnemyId(enemy.definitionId));
            if (!definition.phases.empty()) {
                bossIds.push_back(enemy.id);
            }
        }
    }

    for (const EntityId bossId : bossIds) {
        if (!state.hasEntity(bossId) || !state.entity(bossId).isAlive()) {
            continue;
        }
        const EnemyDefinition& definition = enemyDatabase_.get(EnemyId(state.entity(bossId).definitionId));
        EnemyAiState& aiState = mutableAiStateFor(state, bossId);
        if (aiState.activePhaseIndex < 0 ||
            aiState.activePhaseIndex >= static_cast<int>(definition.phases.size()) ||
            aiState.lastArenaEffectTurn == state.turn) {
            continue;
        }

        const EnemyPhaseDefinition& phase = definition.phases[static_cast<std::size_t>(aiState.activePhaseIndex)];
        aiState.lastArenaEffectTurn = state.turn;
        if (phase.playerTurnEffects.empty()) {
            continue;
        }

        state.log.add(
            CombatLogEntryType::BossArenaEffect,
            {
                {"boss", definition.id.value},
                {"boss_text_id", definition.nameTextId.value},
                {"phase", phase.id},
                {"phase_text_id", phase.nameTextId.value}
            }
        );
        applyEffects(effectSystem_, state, bossId, phase.playerTurnEffects, random, phase.id);
    }
}
