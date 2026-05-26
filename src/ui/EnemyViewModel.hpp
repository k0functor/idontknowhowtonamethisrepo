#pragma once

#include "entities/EntityId.hpp"
#include "enemies/EnemyIntent.hpp"
#include "ui/StatusViewModel.hpp"

#include <string>
#include <vector>

struct EnemyViewModel {
    EntityId entityId;

    std::string name;

    int currentHp = 1;
    int maxHp = 1;
    int block = 0;

    std::string blockLabel = "Block";

    EnemyIntent intent;
    std::string intentText;

    std::vector<StatusViewModel> statuses;

    bool alive = true;
};
