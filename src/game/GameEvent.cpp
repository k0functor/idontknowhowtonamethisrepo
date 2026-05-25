#include "GameEvent.hpp"

#include <stdexcept>
#include <string_view>

std::string toString(const GameEventType type) {
    switch (type) {
        case GameEventType::CombatStarted:
            return "combat_started";
        case GameEventType::CombatEnded:
            return "combat_ended";
        case GameEventType::TurnStarted:
            return "turn_started";
        case GameEventType::TurnEnded:
            return "turn_ended";
        case GameEventType::CardPlayed:
            return "card_played";
        case GameEventType::DamageDealt:
            return "damage_dealt";
        case GameEventType::DamageTaken:
            return "damage_taken";
        case GameEventType::BlockGained:
            return "block_gained";
        case GameEventType::StatusApplied:
            return "status_applied";
        case GameEventType::EnemyKilled:
            return "enemy_killed";
        case GameEventType::RewardGenerated:
            return "reward_generated";
        case GameEventType::GoldGained:
            return "gold_gained";
        case GameEventType::RelicCollected:
            return "relic_collected";
    }

    throw std::runtime_error("Unknown GameEventType");
}

GameEventType gameEventTypeFromString(const std::string_view value) {
    if (value == "combat_started") return GameEventType::CombatStarted;
    if (value == "combat_ended") return GameEventType::CombatEnded;
    if (value == "turn_started") return GameEventType::TurnStarted;
    if (value == "turn_ended") return GameEventType::TurnEnded;
    if (value == "card_played") return GameEventType::CardPlayed;
    if (value == "damage_dealt") return GameEventType::DamageDealt;
    if (value == "damage_taken") return GameEventType::DamageTaken;
    if (value == "block_gained") return GameEventType::BlockGained;
    if (value == "status_applied") return GameEventType::StatusApplied;
    if (value == "enemy_killed") return GameEventType::EnemyKilled;
    if (value == "reward_generated") return GameEventType::RewardGenerated;
    if (value == "gold_gained") return GameEventType::GoldGained;
    if (value == "relic_collected") return GameEventType::RelicCollected;

    throw std::runtime_error("Unknown game event type: " + std::string(value));
}
