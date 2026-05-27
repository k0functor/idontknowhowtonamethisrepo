#pragma once

#include <string>
#include <vector>

struct RunActorState {
    std::string definitionId;
    int currentHp = 1;
    int maxHp = 1;

    int stress = 0;
    int maxStress = 200;
    bool resolveCheckTriggered = false;

    std::vector<std::string> traitIds;
};
