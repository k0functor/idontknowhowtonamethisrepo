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
    int valueMin = 0;
    int valueMax = 0;
};
