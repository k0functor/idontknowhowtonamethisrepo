#pragma once

#include "dice/DiceExpression.hpp"

#include <string>

class EffectValue {
public:
    enum class Type {
        Fixed,
        Dice
    };

    static EffectValue fixed(const int amount);
    static EffectValue dice(const DiceExpression& expression);

    Type type() const;

    bool isFixed() const;
    bool isDice() const;

    int fixedAmount() const;
    const DiceExpression& diceExpression() const;

    int minimumPossibleValue() const;
    int maximumPossibleValue() const;

private:
    Type type_ = Type::Fixed;

    int fixedAmount_ = 0;
    DiceExpression diceExpression_;
};