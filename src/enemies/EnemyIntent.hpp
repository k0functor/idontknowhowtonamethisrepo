#pragma once

#include "effects/EffectTarget.hpp"
#include "effects/EffectType.hpp"

#include <string>
#include <string_view>
#include <vector>

enum class EnemyIntentType {
    Attack,
    Block,
    Buff,
    Debuff,
    Special,
    Unknown
};

std::string toString(EnemyIntentType type);
EnemyIntentType enemyIntentTypeFromString(std::string_view value);

struct EnemyIntentEffectSummary {
    EffectType type = EffectType::Damage;
    EffectTarget target = EffectTarget::Self;

    int valueMin = 0;
    int valueMax = 0;
    int repeatCount = 1;

    std::string statusId;
};

struct EnemyIntent {
    EnemyIntentType type = EnemyIntentType::Unknown;

    // For repeated attacks this is per-hit damage and hitCount is the number of hits.
    // For non-repeated or mixed attacks hitCount stays 1 and valueMin/valueMax are total visible value.
    int valueMin = 0;
    int valueMax = 0;
    int hitCount = 1;

    // Inspect-facing summaries of the actual effects the action will apply.
    // The compact intent label stays simple, while the inspect panel can show
    // target scope, statuses, energy loss and other non-damage consequences.
    std::vector<EnemyIntentEffectSummary> effectSummaries;
};
