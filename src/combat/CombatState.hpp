#pragma once

#include "cards/Deck.hpp"
#include "cards/Hand.hpp"
#include "combat/CombatLog.hpp"
#include "combat/CombatPhase.hpp"
#include "combat/CombatResources.hpp"
#include "entities/CombatEntity.hpp"
#include "combat/EnemyIntentState.hpp"

#include <string>
#include <vector>

struct DroneSlot {
    std::string droneId;
    EntityId owner;
};

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

    // Used by drone-based archetypes. Three slots by default.
    // New summons overflow by using/removing the oldest drone first.
    // Each slot stores a data-driven drone id plus the actor that summoned it.
    std::vector<DroneSlot> droneSlots;
    std::size_t maxDroneSlots = 3;

    Deck deck;
    Hand hand;

    CombatResources resources;
    CombatPhase phase = CombatPhase::NotStarted;
    int turn = 0;

    CombatLog log;
};
