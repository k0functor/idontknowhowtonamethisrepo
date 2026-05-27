#pragma once

#include <string>
#include <string_view>

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

struct EnemyIntent {
    EnemyIntentType type = EnemyIntentType::Unknown;

    // For repeated attacks this is per-hit damage and hitCount is the number of hits.
    // For non-repeated or mixed attacks hitCount stays 1 and valueMin/valueMax are total visible value.
    int valueMin = 0;
    int valueMax = 0;
    int hitCount = 1;
};
