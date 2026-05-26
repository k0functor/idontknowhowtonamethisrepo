#pragma once

#include "entities/EntityId.hpp"
#include "ui/StatusViewModel.hpp"

#include <string>
#include <vector>

struct PlayerViewModel {
    EntityId entityId;

    std::string name;

    int currentHp = 1;
    int maxHp = 1;
    int currentEnergy = 0;
    int maxEnergy = 0;
    int block = 0;

    std::string blockLabel = "Block";

    std::vector<StatusViewModel> statuses;

    bool alive = true;
};
