#pragma once

#include "actors/PlayerActorId.hpp"
#include "localization/TextId.hpp"

#include <string>
#include <vector>

struct PlayerActorDefinition {
    PlayerActorId id;

    TextId nameTextId;
    TextId descriptionTextId;

    int maxHp = 1;
    int startingEnergy = 3;

    std::vector<std::string> startingTraitIds;
    std::vector<std::string> startingRelicIds;
};
