#include "DiceCorruption.hpp"

#include <stdexcept>

std::string toString(const DiceCorruptionType type) {
    switch (type) {
        case DiceCorruptionType::None:
            return "none";
        case DiceCorruptionType::Cursed:
            return "cursed";
        case DiceCorruptionType::Fire:
            return "fire";
        case DiceCorruptionType::Poison:
            return "poison";
        case DiceCorruptionType::Blood:
            return "blood";
        case DiceCorruptionType::Unstable:
            return "unstable";
    }

    throw std::runtime_error("Unknown DiceCorruptionType");
}

std::string toString(const DiceCorruption& corruption) {
    return toString(corruption.type);
}

DiceCorruptionType diceCorruptionTypeFromString(const std::string_view value) {
    if (value == "none" || value == "None") {
        return DiceCorruptionType::None;
    }

    if (value == "cursed" || value == "Cursed") {
        return DiceCorruptionType::Cursed;
    }

    if (value == "fire" || value == "Fire") {
        return DiceCorruptionType::Fire;
    }

    if (value == "poison" || value == "Poison") {
        return DiceCorruptionType::Poison;
    }

    if (value == "blood" || value == "Blood") {
        return DiceCorruptionType::Blood;
    }

    if (value == "unstable" || value == "Unstable") {
        return DiceCorruptionType::Unstable;
    }

    throw std::runtime_error("Unknown dice corruption type: " + std::string(value));
}
