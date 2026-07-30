#pragma once

#include "enemies/EnemyActionDefinition.hpp"
#include "enemies/EnemyId.hpp"
#include "enemies/EnemyRole.hpp"
#include "enemies/EnemyPhaseDefinition.hpp"
#include "localization/TextId.hpp"

#include <vector>

struct EnemyDefinition {
    EnemyId id;
    TextId nameTextId;

    int maxHp = 1;
    int startingBlock = 0;
    EnemyRole role = EnemyRole::Striker;

    std::vector<EnemyActionDefinition> actions;
    std::vector<EnemyPhaseDefinition> phases;
};
