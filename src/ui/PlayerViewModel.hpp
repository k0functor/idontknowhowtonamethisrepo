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
    int stress = 0;
    int maxStress = 200;

    std::string blockLabel = "Block";
    std::string stressLabel = "Stress";
    std::string activeTurnLabel = "Acting";

    std::vector<StatusViewModel> statuses;
    std::vector<std::string> traitIds;

    bool alive = true;
    bool activeTurn = false;
};
