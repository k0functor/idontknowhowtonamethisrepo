#pragma once

#include "combat/CombatState.hpp"
#include "entities/EntityId.hpp"
#include "localization/LocalizationManager.hpp"
#include "statuses/StatusDatabase.hpp"
#include "drones/DroneDatabase.hpp"
#include "data/CardDatabase.hpp"
#include "ui/CardViewModelBuilder.hpp"
#include "ui/CombatViewModel.hpp"
#include "ui/StatusViewModel.hpp"

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

class CombatViewModelBuilder {
public:
    CombatViewModelBuilder(
        const LocalizationManager& localization,
        const StatusDatabase& statusDatabase,
        const DroneDatabase& droneDatabase,
        const CardDatabase& cardDatabase,
        const CardViewModelBuilder& cardViewModelBuilder
    );

    CombatViewModel build(
        const CombatState& state,
        EntityId source,
        std::optional<EntityId> previewTarget
    ) const;

    CombatViewModel build(
        const CombatState& state,
        EntityId source,
        std::optional<EntityId> previewTarget,
        const std::function<EntityId(const CardInstance&)>& sourceForCard
    ) const;

private:
    std::vector<StatusViewModel> buildStatuses(const StatusContainer& statuses) const;

    std::vector<std::string> recentLogEntries(const CombatState& state, std::size_t maxCount) const;

private:
    const LocalizationManager& localization_;
    const StatusDatabase& statusDatabase_;
    const DroneDatabase& droneDatabase_;
    const CardDatabase& cardDatabase_;
    const CardViewModelBuilder& cardViewModelBuilder_;
};
