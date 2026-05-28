#pragma once

#include "run/RunMapNode.hpp"

#include <string>
#include <vector>

struct EncounterDefinition {
    std::string id;
    RunMapNodeType nodeType = RunMapNodeType::Combat;
    std::vector<std::string> enemyIds;
    int weight = 1;
};
