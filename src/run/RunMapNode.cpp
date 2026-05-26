#include "RunMapNode.hpp"

std::string toString(const RunMapNodeType type) {
    switch (type) {
        case RunMapNodeType::Combat:
            return "Combat";
        case RunMapNodeType::Elite:
            return "Elite";
        case RunMapNodeType::Event:
            return "Event";
        case RunMapNodeType::Shop:
            return "Shop";
        case RunMapNodeType::Chest:
            return "Chest";
        case RunMapNodeType::Rest:
            return "Rest";
        case RunMapNodeType::Boss:
            return "Boss";
    }

    return "Unknown";
}
