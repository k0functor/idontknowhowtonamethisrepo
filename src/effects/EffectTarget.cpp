#include "EffectTarget.hpp"

#include <stdexcept>

std::string toString(const EffectTarget& target) {
    switch (target) {
        case EffectTarget::Self:
            return "self";
        case EffectTarget::SingleEnemy:
            return "single_enemy";
        case EffectTarget::AllEnemies:
            return "all_enemies";
        case EffectTarget::RandomEnemy:
            return "random_enemy";
        case EffectTarget::Ally:
            return "ally";
        case EffectTarget::AllAllies:
            return "all_allies";
        case EffectTarget::RandomAlly:
            return "random_ally";
    }

    throw std::runtime_error("Unknown EffectTarget");
}

EffectTarget effectTargetFromString(const std::string_view value) {
    if (value == "self" || value == "Self") {
        return EffectTarget::Self;
    }

    if (value == "single_enemy" || value == "SingleEnemy") {
        return EffectTarget::SingleEnemy;
    }

    if (value == "all_enemies" || value == "AllEnemies") {
        return EffectTarget::AllEnemies;
    }

    if (value == "random_enemy" || value == "RandomEnemy") {
        return EffectTarget::RandomEnemy;
    }

    if (value == "ally" || value == "Ally") {
        return EffectTarget::Ally;
    }

    if (value == "all_allies" || value == "AllAllies") {
        return EffectTarget::AllAllies;
    }

    if (value == "random_ally" || value == "RandomAlly") {
        return EffectTarget::RandomAlly;
    }

    throw std::runtime_error("Unknown effect target: " + std::string(value));
}
