#include "EffectResolver.hpp"

#include "dice/DieType.hpp"

#include <stdexcept>

ResolvedEffectValue EffectResolver::resolveForApply(
    const EffectValue& value,
    const EffectContext& context
) const {
    ResolvedEffectValue result = resolveForPreview(value);

    if (value.isFixed()) {
        result.actual = value.fixedAmount();
        return result;
    }

    if (context.random == nullptr) {
        throw std::runtime_error("EffectResolver needs Random to resolve dice value");
    }

    const DiceExpression& expression = value.diceExpression();
    int total = expression.bonus;

    for (int i = 0; i < expression.count; ++i) {
        total += context.random->rangeInclusive(1, sidesOfDie(expression.dieType));
    }

    result.actual = total;
    return result;
}

ResolvedEffectValue EffectResolver::resolveForPreview(const EffectValue& value) const {
    ResolvedEffectValue result;
    result.minimum = value.minimumPossibleValue();
    result.maximum = value.maximumPossibleValue();
    result.actual = result.minimum;
    result.random = value.isDice();
    return result;
}
