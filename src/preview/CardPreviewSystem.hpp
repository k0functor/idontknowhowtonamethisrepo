#pragma once

#include "combat/BlockSystem.hpp"
#include "combat/CardPlayValidator.hpp"
#include "combat/DamageSystem.hpp"
#include "combat/EffectResolver.hpp"
#include "combat/ModifierSystem.hpp"
#include "data/CardDatabase.hpp"
#include "entities/EntityId.hpp"
#include "preview/CardPreview.hpp"

#include <optional>

class CardPreviewSystem {
public:
    CardPreviewSystem(
        const CardDatabase& cardDatabase,
        const CardPlayValidator& validator,
        const EffectResolver& effectResolver,
        const DamageSystem& damageSystem,
        const BlockSystem& blockSystem
    );

    CardPreview previewCard(
        const CombatState& state,
        CardInstanceId cardInstanceId,
        EntityId source,
        std::optional<EntityId> target
    ) const;

private:
    const CardDatabase& cardDatabase_;
    const CardPlayValidator& validator_;
    const EffectResolver& effectResolver_;
    const DamageSystem& damageSystem_;
    const BlockSystem& blockSystem_;
};
