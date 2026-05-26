#pragma once

#include "drones/DroneActionDefinition.hpp"
#include "drones/DroneId.hpp"
#include "localization/TextId.hpp"

#include <optional>

struct DroneDefinition {
    DroneId id;
    TextId nameTextId;
    TextId descriptionTextId;

    std::optional<DroneActionDefinition> manualAction;
    std::optional<DroneActionDefinition> endTurnAction;
};
