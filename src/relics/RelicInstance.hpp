#pragma once

#include "relics/RelicId.hpp"

struct RelicInstance {
    RelicId id;

    int triggersThisCombat = 0;
    int totalTriggers = 0;
};
