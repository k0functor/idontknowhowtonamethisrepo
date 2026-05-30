#pragma once

#include "archetypes/PlayableArchetypeId.hpp"
#include "localization/TextId.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct ArchetypePaletteDefinition {
    TextId nameTextId;

    std::uint8_t accentR = 100;
    std::uint8_t accentG = 110;
    std::uint8_t accentB = 145;
};

struct PlayableArchetypeDefinition {
    PlayableArchetypeId id;

    TextId nameTextId;
    TextId shortDescriptionTextId;
    TextId detailsDescriptionTextId;
    TextId uniqueMechanicTextId;
    TextId visualIdentityTextId;

    ArchetypePaletteDefinition palette;

    std::vector<std::string> actorDefinitionIds;
    std::vector<std::string> startingDeckCardIds;
    std::vector<std::string> startingRelicIds;
    std::vector<std::string> startingConsumableIds;

    int startingGold = 0;

    std::vector<TextId> strengthTextIds;
    std::vector<TextId> weaknessTextIds;

    std::string mechanicId = "default";

    int selectionOrder = 1000;
};
