#pragma once

#include "effects/EffectDefinition.hpp"

#include <string>
#include <vector>

struct DroneActionDefinition {
    std::vector<EffectDefinition> effects;
    std::string logTextId;
    std::string fallbackLog;
};
