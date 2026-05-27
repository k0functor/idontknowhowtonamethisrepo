#pragma once

#include <string>

struct RunActorState {
    std::string definitionId;
    int currentHp = 1;
    int maxHp = 1;
};
