#pragma once

#include "enemies/EnemyIntent.hpp"
#include "entities/EntityId.hpp"

#include <string>

struct EnemyIntentState {
    EntityId enemyId;
    std::string actionId;
    EnemyIntent intent;
};
