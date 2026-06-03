#pragma once

#include "cards/Deck.hpp"
#include "cards/Hand.hpp"
#include "combat/CombatLog.hpp"
#include "combat/CombatPhase.hpp"
#include "combat/CombatResources.hpp"
#include "entities/CombatEntity.hpp"
#include "combat/EnemyIntentState.hpp"

#include <optional>
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

    std::optional<EntityId> activePlayerId() const;
    CombatEntity* activePlayer();
    const CombatEntity* activePlayer() const;

    std::vector<CombatEntity> players;
    std::vector<CombatEntity> enemies;
    std::vector<EnemyIntentState> enemyIntents;

    // Used by drone-based archetypes. Three slots by default.
    // New summons overflow by removing the oldest drone first.
    // Each slot stores a data-driven drone id plus the actor that summoned it.
    // Drones passively act at the end of the player turn.
    // Card-driven activation triggers the active effect and consumes the drone.
    std::vector<DroneSlot> droneSlots;
    std::size_t maxDroneSlots = 3;

    Deck deck;
    Hand hand;

    CombatResources resources;
    CombatPhase phase = CombatPhase::NotStarted;
    int turn = 0;

    // Most archetypes use one player turn per round. This flag is reserved for
    // mechanics that explicitly need forced actor subturns. Sadist/Masochist
    // uses it to play one shared hand in a fixed order: Sadist, then
    // Masochist, then the enemies.
    bool useSequentialPlayerTurns = false;
    std::size_t activePlayerIndex = 0;

    CombatLog log;
};
