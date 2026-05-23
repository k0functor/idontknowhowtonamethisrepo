#include "DiceExpression.hpp"

#include <string>

std::string toString(const DiceExpression& expression) {
    std::string result = std::to_string(expression.count) + toString(expression.dieType);

    if (expression.bonus > 0) {
        result += "+" + std::to_string(expression.bonus);
    } else if (expression.bonus < 0) {
        result += std::to_string(expression.bonus);
    }

    return result;
}