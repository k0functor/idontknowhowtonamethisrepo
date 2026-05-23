#include "DiceCorruption.hpp"

#include <stdexcept>

std::string toString(const DiceCorruption& corruption)
{
    switch (corruption)
    {
        case DiceCorruption::None:
            return "None";
        case DiceCorruption::Cursed:
            return "Cursed";
        case DiceCorruption::Fire:
            return "Fire";
        case DiceCorruption::Poison:
            return "Poison";
        case DiceCorruption::Blood:
            return "Blood";
        case DiceCorruption::Unstable:
            return "Unstable";
        default:
            throw std::invalid_argument("Invalid DiceCorruption value");
    }
}

DiceCorruptionType parseDiceCorruptionType(std::string_view value)
{
    if (value == "None")
        return DiceCorruptionType::None;
    else if (value == "Cursed")
        return DiceCorruptionType::Cursed;
    else if (value == "Fire")
        return DiceCorruptionType::Fire;
    else if (value == "Poison")
        return DiceCorruptionType::Poison;
    else if (value == "Blood")
        return DiceCorruptionType::Blood;
    else if (value == "Unstable")
        return DiceCorruptionType::Unstable;
    else
        throw std::invalid_argument("Invalid string for DiceCorruptionType: " + std::string(value));
}
