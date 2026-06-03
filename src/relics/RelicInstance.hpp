#pragma once

#include "relics/RelicId.hpp"

#include <string>

struct RelicInstance {
    RelicId id;
    std::string ownerActorDefinitionId;

    int triggersThisCombat = 0;
    int totalTriggers = 0;
};
