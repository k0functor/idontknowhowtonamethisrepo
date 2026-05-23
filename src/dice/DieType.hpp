#pragma once

#include <string>
#include <string_view>

enum class DieType {
    D4,
    D6,
    D8,
    D10,
    D12,
    D20
};

int sidesOfDie(DieType dieType);

std::string toString(DieType dieType);

DieType dieTypeFromString(std::string_view value);