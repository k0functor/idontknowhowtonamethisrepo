#pragma once

#include "entities/EntityId.hpp"

#include <string>
#include <utility>
#include <vector>

struct EnemyViewModel {
    EntityId entityId;

    std::string name;

    int currentHp = 1;
    int maxHp = 1;
    int block = 0;

    std::vector<std::pair<std::string, int>> statuses;

    bool alive = true;
};
