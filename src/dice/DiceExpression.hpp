#pragma once

#include "DieType.hpp"

#include <string>

struct DiceExpression {
    int count = 1;
    DieType dieType = DieType::D6;
    int bonus = 0;
};

std::string toString(const DiceExpression& expression);

