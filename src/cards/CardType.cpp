#include "CardType.hpp"

#include <stdexcept>

std::string toString(const CardType type) {
    switch (type) {
        case CardType::Attack:
            return "attack";
        case CardType::Skill:
            return "skill";
        case CardType::Power:
            return "power";
        case CardType::Status:
            return "status";
        case CardType::Curse:
            return "curse";
    }

    throw std::runtime_error("Unknown CardType");
}

CardType cardTypeFromString(const std::string_view value) {
    if (value == "attack") {
        return CardType::Attack;
    }

    if (value == "skill") {
        return CardType::Skill;
    }

    if (value == "power") {
        return CardType::Power;
    }

    if (value == "status") {
        return CardType::Status;
    }

    if (value == "curse") {
        return CardType::Curse;
    }

    throw std::runtime_error("Unknown card type: " + std::string(value));
}
