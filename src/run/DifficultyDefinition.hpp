#pragma once

#include "localization/TextId.hpp"
#include "run/DifficultyId.hpp"

struct DifficultyDefinition {
    DifficultyId id;

    TextId nameTextId;
    TextId descriptionTextId;

    float enemyHpMultiplier = 1.f;
    float enemyDamageMultiplier = 1.f;
    float goldMultiplier = 1.f;
};
