#pragma once

#include "preview/PreviewValue.hpp"

#include <string>
#include <vector>

struct CardOutcomePreview {
    PreviewValue modifiedDamage;
    PreviewValue hpDamage;
    PreviewValue block;
    PreviewValue healing;

    int affectedEnemyCount = 0;
    int affectedAllyCount = 0;
    bool requiresTargetSelection = false;
    bool usesRandomTarget = false;

    std::vector<std::string> modifierLabels;

    bool hasDamage() const {
        return modifiedDamage.maximum > 0 || hpDamage.maximum > 0;
    }

    bool hasBlock() const {
        return block.maximum > 0;
    }

    bool hasHealing() const {
        return healing.maximum > 0;
    }
};
