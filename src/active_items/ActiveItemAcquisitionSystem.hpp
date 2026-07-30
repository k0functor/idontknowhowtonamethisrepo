#pragma once

#include "active_items/ActiveItemDatabase.hpp"
#include "core/Random.hpp"
#include "run/RunState.hpp"

#include <optional>
#include <string>

class ActiveItemAcquisitionSystem {
public:
    static std::optional<ActiveItemId> chooseReward(
        const ActiveItemDatabase& items,
        const std::string& equippedItemId,
        Random& random
    );

    static std::optional<ActiveItemId> chooseShopOffer(
        const ActiveItemDatabase& items,
        const std::string& equippedItemId,
        Random& random
    );

    static bool equipReplacement(
        RunState& run,
        const ActiveItemDatabase& items,
        const ActiveItemId& itemId
    );
};
