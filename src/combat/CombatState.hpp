#pragma once

#include "cards/Deck.hpp"
#include "cards/Hand.hpp"
#include "combat/CombatLog.hpp"
#include "combat/CombatPhase.hpp"
#include "combat/CombatResources.hpp"
#include "combat/CombatTelemetry.hpp"
#include "entities/CombatEntity.hpp"
#include "combat/EnemyIntentState.hpp"
#include "combat/EnemyAiState.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct ForcedCardPlay {
    CardInstanceId cardInstanceId;
    EntityId source;
    std::optional<EntityId> target;
};

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
    std::size_t aliveEnemyCount() const;
    std::size_t alivePlayerCount() const;

    void pruneEnemyIntents();

    std::optional<EntityId> activePlayerId() const;
    CombatEntity* activePlayer();
    const CombatEntity* activePlayer() const;

    void rememberStatusSeen(const std::string& statusId);
    EntityId createDynamicEntityId() const;

    int cardCostModifier(EntityId owner) const;
    void setCardCostModifier(EntityId owner, int amount);
    void clearTurnCardCostModifiers();

    void primeStressBreakdown(EntityId target, std::string breakdownType);
    std::optional<std::string> consumePrimedStressBreakdown(EntityId target);
    std::optional<std::string> primedStressBreakdown(EntityId target) const;

    void armStressBreakdownGuard(EntityId target);
    bool hasStressBreakdownGuard(EntityId target) const;
    bool consumeStressBreakdownGuard(EntityId target);

    std::vector<CombatEntity> players;
    std::vector<CombatEntity> enemies;
    std::vector<EnemyIntentState> enemyIntents;
    std::vector<EnemyAiState> enemyAiStates;

    std::optional<ForcedCardPlay> pendingForcedCardPlay;

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
    CombatTelemetry telemetry;
    CombatPhase phase = CombatPhase::NotStarted;
    int turn = 0;
    float enemyHpMultiplier = 1.f;
    float enemyDamageMultiplier = 1.f;

    // Most archetypes use one player turn per round. This flag is reserved for
    // mechanics that explicitly need forced actor subturns. Sadist/Masochist
    // uses it to play one shared hand in a fixed order: Sadist, then
    // Masochist, then the enemies.
    bool useSequentialPlayerTurns = false;
    std::size_t activePlayerIndex = 0;

    // Tracks every status that was actually applied during the combat.
    // The final combat result also collects statuses still present at the end,
    // but short-lived statuses can expire before victory/defeat. This log keeps
    // compendium discovery honest without needing a full combat event replay.
    std::vector<std::string> seenStatusIds;

    // Tracks every successfully played card definition during this combat.
    // Profile statistics consume this after victory/defeat, so combat scenes do
    // not need to know about profile saves.
    std::vector<std::string> playedCardIds;

    CombatLog log;

private:
    std::unordered_map<std::uint64_t, int> turnCardCostModifiers_;
    std::unordered_map<std::uint64_t, std::string> primedStressBreakdowns_;
    std::unordered_map<std::uint64_t, bool> stressBreakdownGuards_;
};
