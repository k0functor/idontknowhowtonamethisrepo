#pragma once

#include "combat/CombatState.hpp"
#include "data/CardDatabase.hpp"
#include "entities/EntityId.hpp"
#include "localization/LocalizationManager.hpp"
#include "preview/CardPreviewSystem.hpp"
#include "ui/CardViewModel.hpp"

#include <optional>
#include <string>

class CardViewModelBuilder {
public:
    CardViewModelBuilder(
        const CardDatabase& cardDatabase,
        const LocalizationManager& localization,
        const CardPreviewSystem& previewSystem
    );

    CardViewModel build(
        const CombatState& state,
        CardInstanceId cardInstanceId,
        EntityId source,
        std::optional<EntityId> target
    ) const;

private:
    const CardDatabase& cardDatabase_;
    const LocalizationManager& localization_;
    const CardPreviewSystem& previewSystem_;
};
