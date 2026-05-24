#include "EffectTarget.hpp"

#include <stdexcept>

std::string toString(const EffectTarget& target) {
    switch (target)
    {
        case EffectTarget::Self:
            return "Self";
        case EffectTarget::SingleEnemy:
            return "SingleEnemy";
        case EffectTarget::AllEnemies:
            return "AllEnemies";
        case EffectTarget::RandomEnemy:
            return "RandomEnemy";
        case EffectTarget::Ally:
            return "Ally";
        case EffectTarget::AllAllies:
            return "AllAllies";
        case EffectTarget::RandomAlly:
            return "RandomAlly";
        default:
            throw std::invalid_argument("Invalid EffectTarget value");
    }
}

EffectTarget parseEffectTarget(std::string_view value) {
    if (value == "Self")
        return EffectTarget::Self;
    else if (value == "SingleEnemy")
        return EffectTarget::SingleEnemy;
    else if (value == "AllEnemies")
        return EffectTarget::AllEnemies;
    else if (value == "RandomEnemy")
        return EffectTarget::RandomEnemy;
    else if (value == "Ally")
        return EffectTarget::Ally;
    else if (value == "AllAllies")
        return EffectTarget::AllAllies;
    else if (value == "RandomAlly")
        return EffectTarget::RandomAlly;
    else
        throw std::invalid_argument("Invalid string for EffectTarget: " + std::string(value));
}
