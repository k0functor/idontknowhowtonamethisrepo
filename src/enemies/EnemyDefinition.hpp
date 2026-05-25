#pragma once

#include "enemies/EnemyId.hpp"
#include "localization/TextId.hpp"

struct EnemyDefinition {
    EnemyId id;
    TextId nameTextId;

    int maxHp = 1;
    int startingBlock = 0;
};
