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

    // Number of times this effect is applied. Damage repeat_count=4 means four separate hits,
    // so block, on-hit triggers and future reactive effects see each hit independently.
    int repeatCount = 1;

    std::optional<std::string> statusId;
};