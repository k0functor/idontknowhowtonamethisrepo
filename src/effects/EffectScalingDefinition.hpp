#pragma once

#include <optional>
#include <string>

enum class EffectScalingStatusOwner {
    Source,
    Target
};

struct EffectScalingDefinition {
    std::optional<std::string> statusId;
    EffectScalingStatusOwner statusOwner = EffectScalingStatusOwner::Source;

    int bonusIfStatusPresent = 0;
    int bonusPerStatusStack = 0;
    int bonusPerCardInHand = 0;
    int bonusPerCardInDiscard = 0;

    // -1 means unlimited.
    int maximumBonus = -1;

    bool empty() const {
        return !statusId.has_value() &&
            bonusIfStatusPresent == 0 &&
            bonusPerStatusStack == 0 &&
            bonusPerCardInHand == 0 &&
            bonusPerCardInDiscard == 0;
    }
};
