#pragma once

#include "enemies/EnemyActionDefinition.hpp"
#include "enemies/EnemyId.hpp"
#include "localization/TextId.hpp"

#include <vector>

struct EnemyDefinition {
    EnemyId id;
    TextId nameTextId;

    int maxHp = 1;
    int startingBlock = 0;

    std::vector<EnemyActionDefinition> actions;
};
