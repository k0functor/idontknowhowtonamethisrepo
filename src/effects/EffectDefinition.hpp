#pragma once

#include "EffectType.hpp"
#include "EffectValue.hpp"
#include "EffectTarget.hpp"

#include <optional>
#include <string>

struct EffectDefinition {
    EffectType type = EffectType::Damage;
    EffectTarget target = EffectTarget::SingleEnemy;

    EffectValue value = EffectValue::fixed(0);

    std::optional<std::string> statusId;
};