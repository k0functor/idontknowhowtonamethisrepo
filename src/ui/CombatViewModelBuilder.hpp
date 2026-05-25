#pragma once

#include "combat/CombatState.hpp"
#include "entities/EntityId.hpp"
#include "localization/LocalizationManager.hpp"
#include "statuses/StatusDatabase.hpp"
#include "ui/CardViewModelBuilder.hpp"
#include "ui/CombatViewModel.hpp"
#include "ui/StatusViewModel.hpp"

#include <cstddef>
#include <optional>

class CombatViewModelBuilder {
public:
    CombatViewModelBuilder(
        const LocalizationManager& localization,
        const StatusDatabase& statusDatabase,
        const CardViewModelBuilder& cardViewModelBuilder
    );

    CombatViewModel build(
        const CombatState& state,
        EntityId source,
        std::optional<EntityId> previewTarget
    ) const;

private:
    std::vector<StatusViewModel> buildStatuses(const StatusContainer& statuses) const;

    static std::vector<std::string> recentLogEntries(const CombatState& state, std::size_t maxCount);

private:
    const LocalizationManager& localization_;
    const StatusDatabase& statusDatabase_;
    const CardViewModelBuilder& cardViewModelBuilder_;
};
