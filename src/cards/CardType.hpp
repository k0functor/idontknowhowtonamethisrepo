#pragma once

#include <string>
#include <string_view>

enum class CardType {
    Attack,
    Skill,
    Power,
    Status,
    Curse
};

std::string toString(CardType type);

CardType cardTypeFromString(std::string_view value);
