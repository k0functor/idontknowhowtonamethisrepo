#pragma once

#include "cards/DrawSystem.hpp"
#include "combat/BlockSystem.hpp"
#include "combat/CombatState.hpp"
#include "combat/DamageSystem.hpp"
#include "combat/EffectResolver.hpp"
#include "combat/EnergySystem.hpp"
#include "combat/Targeting.hpp"
#include "core/Random.hpp"
#include "drones/DroneDatabase.hpp"
#include "game/GameEventBus.hpp"
#include "statuses/StatusSystem.hpp"

#include <optional>
#include <string>

class DroneSystem {
public:
    DroneSystem(
        const DroneDatabase& drones,
        const EffectResolver& effectResolver,
        const Targeting& targeting,
        const DamageSystem& damageSystem,
        const BlockSystem& blockSystem,
        const EnergySystem& energySystem,
        const DrawSystem& drawSystem,
        const StatusSystem& statusSystem,
        const GameEventBus* eventBus = nullptr
    );

    bool isKnownDrone(const std::string& droneId) const;

    void summonDrone(
        CombatState& state,
        const std::string& droneId,
        EntityId owner,
        Random* random = nullptr
    ) const;

    void useOldestDrone(
        CombatState& state,
        Random* random = nullptr
    ) const;

    void processEndOfPlayerTurn(
        CombatState& state,
        Random& random
    ) const;

private:
    EntityId validOwnerOrFallback(const CombatState& state, EntityId owner) const;
    std::optional<EntityId> targetForEffect(
        const CombatState& state,
        EffectTarget target,
        EntityId owner
    ) const;

    void applyDroneAction(
        CombatState& state,
        const DroneSlot& slot,
        const DroneActionDefinition& action,
        Random* random
    ) const;

    void applyDroneEffect(
        CombatState& state,
        const EffectDefinition& effect,
        const EffectContext& baseContext
    ) const;

private:
    const DroneDatabase& drones_;
    const EffectResolver& effectResolver_;
    const Targeting& targeting_;
    const DamageSystem& damageSystem_;
    const BlockSystem& blockSystem_;
    const EnergySystem& energySystem_;
    const DrawSystem& drawSystem_;
    const StatusSystem& statusSystem_;
    const GameEventBus* eventBus_ = nullptr;
};
