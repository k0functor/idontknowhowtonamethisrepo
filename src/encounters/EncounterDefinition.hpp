#pragma once

#include "run/RunMapNode.hpp"

#include <string>
#include <vector>

struct EncounterDefinition {
    std::string id;
    RunMapNodeType nodeType = RunMapNodeType::Combat;
    std::vector<std::string> enemyIds;
    int weight = 1;

    // Zero-based map layer limits. -1 means no limit.
    int minLayer = -1;
    int maxLayer = -1;

    bool isAllowedOnLayer(int layerIndex) const {
        if (layerIndex < 0) {
            return true;
        }

        if (minLayer >= 0 && layerIndex < minLayer) {
            return false;
        }

        if (maxLayer >= 0 && layerIndex > maxLayer) {
            return false;
        }

        return true;
    }
};
