#pragma once

#include <string>
#include <vector>

struct ProfileData {
    int slotIndex = 0;
    bool isEmpty = true;

    std::vector<std::string> unlockedCardIds;
    std::vector<std::string> unlockedRelicIds;
    std::vector<std::string> unlockedArchetypeIds;

    int victories = 0;
    int defeats = 0;
};
