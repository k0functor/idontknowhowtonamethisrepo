#include "DieType.hpp"

#include <stdexcept>

int sidesOfDie(const DieType dieType) {
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
    }

    throw std::runtime_error("Unknown DieType");
}

std::string toString(const DieType dieType) {
    switch (dieType) {
        case DieType::D4:
            return "d4";
        case DieType::D6:
            return "d6";
        case DieType::D8:
            return "d8";
        case DieType::D10:
            return "d10";
        case DieType::D12:
            return "d12";
        case DieType::D20:
            return "d20";
    }

    throw std::runtime_error("Unknown DieType");
}

DieType dieTypeFromString(const std::string_view value) {
    if (value == "d4" || value == "D4") {
        return DieType::D4;
    }

    if (value == "d6" || value == "D6") {
        return DieType::D6;
    }

    if (value == "d8" || value == "D8") {
        return DieType::D8;
    }

    if (value == "d10" || value == "D10") {
        return DieType::D10;
    }

    if (value == "d12" || value == "D12") {
        return DieType::D12;
    }

    if (value == "d20" || value == "D20") {
        return DieType::D20;
    }

    throw std::runtime_error("Unknown die type: " + std::string(value));
}
