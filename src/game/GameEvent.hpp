#pragma once

#include "cards/CardId.hpp"
#include "cards/CardInstanceId.hpp"
#include "cards/CardType.hpp"
#include "effects/EffectType.hpp"
#include "entities/EntityId.hpp"

#include <optional>
#include <string>
#include <string_view>

class CombatState;

// GameEvent is intentionally small and value-based for now.
// It is a shared language between systems such as relics, statuses,
// traits, achievements and future challenge tracking.
enum class GameEventType {
    CombatStarted,
    CombatEnded,
    TurnStarted,
    TurnEnded,
    CardPlayed,
    DamageDealt,
    DamageTaken,
    BlockGained,
    Healed,
    StatusApplied,
    EnemyKilled,
    RewardGenerated,
    GoldGained,
    RelicCollected
};

std::string toString(GameEventType type);
GameEventType gameEventTypeFromString(std::string_view value);

struct GameEvent {
    GameEventType type = GameEventType::CombatStarted;

    std::optional<EntityId> source;
    std::optional<EntityId> target;

    std::optional<CardInstanceId> cardInstanceId;
    std::optional<CardId> cardDefinitionId;
    std::optional<CardType> cardType;

    std::string statusId;
    std::string relicId;

    EffectType effectType = EffectType::Damage;
    int amount = 0;
    int blockedAmount = 0;
    int turn = 0;
};
