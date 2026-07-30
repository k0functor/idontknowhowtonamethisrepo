#pragma once

#include "run/RunMapNode.hpp"

#include <cstddef>
#include <string>
#include <vector>

struct EncounterDefinition {
    static constexpr std::size_t MinimumEnemyCount = 1;
    static constexpr std::size_t MaximumEnemyCount = 3;
    std::string id;
    RunMapNodeType nodeType = RunMapNodeType::Combat;
    std::vector<std::string> enemyIds;
    int weight = 1;

    std::size_t enemyCount() const {
        return enemyIds.size();
    }

    bool isGroupEncounter() const {
        return enemyIds.size() > 1;
    }

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
