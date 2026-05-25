#pragma once

#include <string>
#include <string_view>

enum class EffectTarget {
    Self,
    SingleEnemy,
    AllEnemies,
    RandomEnemy,
    Ally,
    AllAllies,
    RandomAlly
};

std::string toString(const EffectTarget& target);

EffectTarget effectTargetFromString(std::string_view value);
