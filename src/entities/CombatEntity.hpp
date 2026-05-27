#pragma once

#include "entities/EntityId.hpp"
#include "entities/EntityType.hpp"
#include "entities/Health.hpp"
#include "entities/StatusContainer.hpp"
#include "localization/TextId.hpp"

#include <string>
#include <vector>

struct CombatEntity {
    EntityId id;
    EntityType type = EntityType::Enemy;

    std::string definitionId;
    TextId nameTextId;

    Health health;
    int block = 0;

    int stress = 0;
    int maxStress = 100;
    std::vector<std::string> traitIds;

    StatusContainer statuses;

    bool isAlive() const {
        return !health.isDead();
    }
};
