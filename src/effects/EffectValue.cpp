#include "EffectValue.hpp"

#include "dice/DieType.hpp"

#include <stdexcept>

EffectValue EffectValue::fixed(const int amount) {
    EffectValue value;
    value.type_ = Type::Fixed;
    value.fixedAmount_ = amount;
    return value;
}

EffectValue EffectValue::dice(const DiceExpression& expression) {
    EffectValue value;
    value.type_ = Type::Dice;
    value.diceExpression_ = expression;
    return value;
}

EffectValue::Type EffectValue::type() const {
    return type_;
}

bool EffectValue::isFixed() const {
    return type_ == Type::Fixed;
}

bool EffectValue::isDice() const {
    return type_ == Type::Dice;
}

int EffectValue::fixedAmount() const {
    if (!isFixed()) {
        throw std::invalid_argument("Effect is not fixed");
    }
    return fixedAmount_;
}

const DiceExpression& EffectValue::diceExpression() const {
    if (!isDice()) {
        throw std::invalid_argument("Effect is not dice");
    }
    return diceExpression_;
}

int EffectValue::minimumPossibleValue() const {
    if (isFixed()) {
        return fixedAmount();
    } else {
        return diceExpression().minimumPossibleValue();
    }
}

int EffectValue::maximumPossibleValue() const {
    if (isFixed()) {
        return fixedAmount();
    } else {
        return diceExpression().maximumPossibleValue();
    }
}