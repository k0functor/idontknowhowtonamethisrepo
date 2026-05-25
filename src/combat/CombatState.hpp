#pragma once

#include "cards/Deck.hpp"
#include "cards/Hand.hpp"
#include "combat/CombatLog.hpp"
#include "combat/CombatPhase.hpp"
#include "combat/CombatResources.hpp"
#include "entities/CombatEntity.hpp"
#include "combat/EnemyIntentState.hpp"

#include <vector>

class CombatState {
public:
    CombatEntity& entity(EntityId id);
    const CombatEntity& entity(EntityId id) const;

    bool hasEntity(EntityId id) const;
    bool isEnemy(EntityId id) const;
    bool isPlayer(EntityId id) const;

    std::vector<EntityId> aliveEnemyIds() const;
    std::vector<EntityId> alivePlayerIds() const;

    std::vector<CombatEntity> players;
    std::vector<CombatEntity> enemies;
    std::vector<EnemyIntentState> enemyIntents;

    Deck deck;
    Hand hand;

    CombatResources resources;
    CombatPhase phase = CombatPhase::NotStarted;
    int turn = 0;

    CombatLog log;
};
