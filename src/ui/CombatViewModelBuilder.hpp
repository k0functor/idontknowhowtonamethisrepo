#pragma once

#include "combat/CombatState.hpp"
#include "entities/EntityId.hpp"
#include "localization/LocalizationManager.hpp"
#include "ui/CardViewModelBuilder.hpp"
#include "ui/CombatViewModel.hpp"

#include <cstddef>
#include <optional>

class CombatViewModelBuilder {
public:
    CombatViewModelBuilder(
        const LocalizationManager& localization,
        const CardViewModelBuilder& cardViewModelBuilder
    );

    CombatViewModel build(
        const CombatState& state,
        EntityId source,
        std::optional<EntityId> previewTarget
    ) const;

private:
    static std::vector<std::string> recentLogEntries(const CombatState& state, std::size_t maxCount);

private:
    const LocalizationManager& localization_;
    const CardViewModelBuilder& cardViewModelBuilder_;
};
