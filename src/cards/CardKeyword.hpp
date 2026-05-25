#pragma once

#include <string>
#include <string_view>

enum class CardKeyword {
    Exhaust,
    Retain,
    Ethereal,
    Innate,
    Unplayable
};

std::string toString(CardKeyword keyword);

CardKeyword cardKeywordFromString(std::string_view value);
