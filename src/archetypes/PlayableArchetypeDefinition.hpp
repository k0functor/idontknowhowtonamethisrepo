#pragma once

#include "archetypes/PlayableArchetypeId.hpp"
#include "localization/TextId.hpp"

#include <string>
#include <vector>

struct PlayableArchetypeDefinition {
    PlayableArchetypeId id;

    TextId nameTextId;
    TextId shortDescriptionTextId;
    TextId detailsDescriptionTextId;
    TextId uniqueMechanicTextId;

    std::vector<std::string> actorDefinitionIds;
    std::vector<std::string> startingDeckCardIds;
    std::vector<std::string> startingRelicIds;
    std::vector<std::string> startingConsumableIds;

    int startingGold = 0;

    std::vector<TextId> strengthTextIds;
    std::vector<TextId> weaknessTextIds;

    std::string mechanicId = "default";
};
