#pragma once

#include "cards/CardId.hpp"
#include "consumables/ConsumableDatabase.hpp"
#include "core/Random.hpp"
#include "data/CardDatabase.hpp"
#include "run/RunState.hpp"

#include <optional>
#include <string>

class ActiveItemContextSystem {
public:
    static std::optional<std::string> createRandomConsumable(
        RunState& run,
        const ConsumableDatabase& consumables,
        Random& random
    );

    static int rerollAvailableMapNodes(RunState& run, Random& random);

    static bool copyCard(
        RunState& run,
        const CardDatabase& cards,
        const CardId& cardId
    );
};
