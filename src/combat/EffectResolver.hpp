#pragma once

#include "combat/EffectContext.hpp"
#include "combat/ResolvedEffectValue.hpp"
#include "effects/EffectValue.hpp"

class EffectResolver {
public:
    ResolvedEffectValue resolveForApply(
        const EffectValue& value,
        const EffectContext& context
    ) const;

    ResolvedEffectValue resolveForPreview(
        const EffectValue& value
    ) const;
};
