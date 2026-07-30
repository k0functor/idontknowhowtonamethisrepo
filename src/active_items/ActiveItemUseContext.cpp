#include "ActiveItemUseContext.hpp"

#include <stdexcept>

ActiveItemUseContext activeItemUseContextFromString(const std::string& value) {
    if (value == "map") return ActiveItemUseContext::Map;
    if (value == "combat") return ActiveItemUseContext::Combat;
    if (value == "reward") return ActiveItemUseContext::Reward;
    if (value == "shop") return ActiveItemUseContext::Shop;
    if (value == "chest") return ActiveItemUseContext::Chest;
    if (value == "event") return ActiveItemUseContext::Event;
    if (value == "rest") return ActiveItemUseContext::Rest;
    throw std::runtime_error("Unknown active item use context: " + value);
}

std::string toString(const ActiveItemUseContext context) {
    switch (context) {
        case ActiveItemUseContext::Map: return "map";
        case ActiveItemUseContext::Combat: return "combat";
        case ActiveItemUseContext::Reward: return "reward";
        case ActiveItemUseContext::Shop: return "shop";
        case ActiveItemUseContext::Chest: return "chest";
        case ActiveItemUseContext::Event: return "event";
        case ActiveItemUseContext::Rest: return "rest";
    }
    return "map";
}
