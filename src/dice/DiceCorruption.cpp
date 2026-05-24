#include "DiceCorruption.hpp"

#include <stdexcept>

std::string toString(const DiceCorruptionType& corruption)
{
    switch (corruption)
    {
        case DiceCorruptionType::None:
            return "None";
        case DiceCorruptionType::Cursed:
            return "Cursed";
        case DiceCorruptionType::Fire:
            return "Fire";
        case DiceCorruptionType::Poison:
            return "Poison";
        case DiceCorruptionType::Blood:
            return "Blood";
        case DiceCorruptionType::Unstable:
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
