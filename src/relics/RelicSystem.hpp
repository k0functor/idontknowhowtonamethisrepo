#pragma once

#include "combat/EffectSystem.hpp"
#include "core/Random.hpp"
#include "game/GameEvent.hpp"
#include "relics/RelicDatabase.hpp"
#include "relics/RelicInventory.hpp"
#include "run/RunState.hpp"

#include <optional>


class RelicSystem final {
public:
    explicit RelicSystem(const RelicDatabase& database);

    void setRelics(const std::vector<std::string>& relicIds);
    void setRelics(const RunState& run);
    void clear();

    const RelicInventory& inventory() const;
    RelicInventory& inventory();

    void startCombat();

    void handleEvent(
        CombatState& state,
        const GameEvent& event,
        const EffectSystem& effectSystem,
        Random& random
    );

private:
    void prepareCardTracking(
        const CombatState& state,
        const GameEvent& event,
        RelicInstance& instance,
        std::optional<EntityId> ownerSource
    ) const;

    bool triggerMatches(
        const CombatState& state,
        const RelicTriggerDefinition& trigger,
        const GameEvent& event,
        const RelicInstance& instance,
        std::optional<EntityId> ownerSource
    ) const;

    std::optional<EntityId> ownerSource(const CombatState& state, const RelicInstance& instance) const;
    std::optional<EntityId> conditionOwner(const CombatState& state, const GameEvent& event, std::optional<EntityId> ownerSource) const;
    int droneCountForOwner(const CombatState& state, std::optional<EntityId> ownerSource) const;
    EntityId defaultPlayerSource(const CombatState& state) const;

private:
    const RelicDatabase& database_;
    RelicInventory inventory_;
};
