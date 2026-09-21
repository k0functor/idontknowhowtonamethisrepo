#pragma once

#include <cstdint>
#include <vector>

class Random;
struct CardDefinition;

class CardRewardQuality {
public:
    static std::vector<const CardDefinition*> chooseOffers(
        std::vector<const CardDefinition*> candidates,
        const std::vector<const CardDefinition*>& deck,
        int count,
        Random& random
    );

    static int relevanceScore(
        const CardDefinition& candidate,
        const std::vector<const CardDefinition*>& deck
    );

    static std::uint32_t themeMask(const CardDefinition& card);
};
