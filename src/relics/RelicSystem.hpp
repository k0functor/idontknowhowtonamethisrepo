#pragma once

#include "combat/EffectSystem.hpp"
#include "combat/ModifierSystem.hpp"
#include "core/Random.hpp"
#include "game/GameEvent.hpp"
#include "relics/RelicDatabase.hpp"
#include "relics/RelicInventory.hpp"
#include "run/RunState.hpp"

#include <optional>

class LocalizationManager;

class RelicSystem final : public IModifierProvider {
public:
    RelicSystem(const RelicDatabase& database, const LocalizationManager& localization);

    void setRelics(const std::vector<std::string>& relicIds);
    void setRelics(const RunState& run);
    void clear();

    const RelicInventory& inventory() const;
    RelicInventory& inventory();

    void startCombat();

    void collectModifiers(
        const CombatState& state,
        const ModifierContext& context,
        std::vector<ValueModifier>& output
    ) const override;

    void handleEvent(
        CombatState& state,
        const GameEvent& event,
        const EffectSystem& effectSystem,
        Random& random
    );

private:
    bool triggerMatches(
        const CombatState& state,
        const RelicTriggerDefinition& trigger,
        const GameEvent& event,
        const RelicInstance& instance,
        std::optional<EntityId> ownerSource
    ) const;

    std::optional<EntityId> ownerSource(const CombatState& state, const RelicInstance& instance) const;
    EntityId defaultPlayerSource(const CombatState& state) const;

private:
    const RelicDatabase& database_;
    const LocalizationManager& localization_;
    RelicInventory inventory_;
};
