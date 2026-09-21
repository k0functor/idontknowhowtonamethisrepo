#pragma once

#include "shop/ShopState.hpp"

class ActiveItemDatabase;
class CardDatabase;
class ConsumableDatabase;
class Random;
class RelicDatabase;
class RunState;
class ShopTuning;

class ShopGenerator {
public:
    static ShopState createShop(
        const RunState& run,
        const CardDatabase& cards,
        const RelicDatabase& relics,
        const ConsumableDatabase& consumables,
        const ActiveItemDatabase& activeItems,
        const ShopTuning& tuning,
        Random& random
    );

    static ShopState createMerchantRest(
        const RunState& run,
        const CardDatabase& cards,
        const ShopTuning& tuning,
        Random& random
    );
};
