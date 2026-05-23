#include "DieType.hpp"

#include <stdexcept>

int sidesOfDie(DieType dieType) {
    switch (dieType) {
        case DieType::D4:
            return 4;
        case DieType::D6:
            return 6;
        case DieType::D8:
            return 8;
        case DieType::D10:
            return 10;
        case DieType::D12:
            return 12;
        case DieType::D20:
            return 20;
        default:
            throw std::invalid_argument("Invalid die type");
    }
}

std::string toString(DieType dieType) {
    switch (dieType) {
        case DieType::D4:
            return "D4";
        case DieType::D6:
            return "D6";
        case DieType::D8:
            return "D8";
        case DieType::D10:
            return "D10";
        case DieType::D12:
            return "D12";
        case DieType::D20:
            return "D20";
        default:
            throw std::invalid_argument("Invalid die type");
    }
}

DieType dieTypeFromString(std::string_view value) {
    if (value == "D4") {
        return DieType::D4;
    } else if (value == "D6") {
        return DieType::D6;
    } else if (value == "D8") {
        return DieType::D8;
    } else if (value == "D10") {
        return DieType::D10;
    } else if (value == "D12") {
        return DieType::D12;
    } else if (value == "D20") {
        return DieType::D20;
    } else {
        throw std::invalid_argument("Invalid die type string");
    }
}