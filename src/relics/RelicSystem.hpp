#pragma once

#include "combat/EffectSystem.hpp"
#include "combat/ModifierSystem.hpp"
#include "core/Random.hpp"
#include "game/GameEvent.hpp"
#include "relics/RelicDatabase.hpp"
#include "relics/RelicInventory.hpp"

class RelicSystem final : public IModifierProvider {
public:
    explicit RelicSystem(const RelicDatabase& database);

    void setRelics(const std::vector<std::string>& relicIds);
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
        const RelicTriggerDefinition& trigger,
        const GameEvent& event,
        const RelicInstance& instance
    ) const;

    EntityId defaultPlayerSource(const CombatState& state) const;

private:
    const RelicDatabase& database_;
    RelicInventory inventory_;
};
