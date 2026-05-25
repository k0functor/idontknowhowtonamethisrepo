#include "EnemyIntent.hpp"

#include <stdexcept>

std::string toString(const EnemyIntentType type) {
    switch (type) {
        case EnemyIntentType::Attack:
            return "attack";
        case EnemyIntentType::Block:
            return "block";
        case EnemyIntentType::Buff:
            return "buff";
        case EnemyIntentType::Debuff:
            return "debuff";
        case EnemyIntentType::Special:
            return "special";
        case EnemyIntentType::Unknown:
            return "unknown";
    }

    throw std::runtime_error("Unknown EnemyIntentType");
}

EnemyIntentType enemyIntentTypeFromString(const std::string_view value) {
    if (value == "attack" || value == "Attack") {
        return EnemyIntentType::Attack;
    }

    if (value == "block" || value == "Block") {
        return EnemyIntentType::Block;
    }

    if (value == "buff" || value == "Buff") {
        return EnemyIntentType::Buff;
    }

    if (value == "debuff" || value == "Debuff") {
        return EnemyIntentType::Debuff;
    }

    if (value == "special" || value == "Special") {
        return EnemyIntentType::Special;
    }

    if (value == "unknown" || value == "Unknown") {
        return EnemyIntentType::Unknown;
    }

    throw std::runtime_error("Unknown enemy intent type: " + std::string(value));
}
