#pragma once

#include "combat/DamagePreview.hpp"
#include "effects/EffectTarget.hpp"
#include "effects/EffectType.hpp"
#include "preview/PreviewValue.hpp"

#include <optional>
#include <string>

struct EffectPreview {
    EffectType type = EffectType::Damage;
    EffectTarget target = EffectTarget::SingleEnemy;

    PreviewValue value;
    int repeatCount = 1;
    std::optional<DamagePreview> damage;

    std::optional<std::string> statusId;
};
