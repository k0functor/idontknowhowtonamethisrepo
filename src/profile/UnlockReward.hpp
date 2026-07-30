#pragma once

#include <string>
#include <vector>

struct UnlockReward {
    std::vector<std::string> archetypeIds;
    std::vector<std::string> cardIds;
    std::vector<std::string> relicIds;

    bool empty() const {
        return archetypeIds.empty() && cardIds.empty() && relicIds.empty();
    }
};
