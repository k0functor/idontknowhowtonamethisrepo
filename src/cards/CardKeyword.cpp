#include "CardKeyword.hpp"

#include <stdexcept>

std::string toString(const CardKeyword keyword) {
    switch (keyword) {
        case CardKeyword::Exhaust:
            return "exhaust";
        case CardKeyword::Retain:
            return "retain";
        case CardKeyword::Ethereal:
            return "ethereal";
        case CardKeyword::Innate:
            return "innate";
        case CardKeyword::Unplayable:
            return "unplayable";
    }

    throw std::runtime_error("Unknown CardKeyword");
}

CardKeyword cardKeywordFromString(const std::string_view value) {
    if (value == "exhaust") {
        return CardKeyword::Exhaust;
    }

    if (value == "retain") {
        return CardKeyword::Retain;
    }

    if (value == "ethereal") {
        return CardKeyword::Ethereal;
    }

    if (value == "innate") {
        return CardKeyword::Innate;
    }

    if (value == "unplayable") {
        return CardKeyword::Unplayable;
    }

    throw std::runtime_error("Unknown card keyword: " + std::string(value));
}
