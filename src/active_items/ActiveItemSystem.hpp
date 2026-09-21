#pragma once

#include "active_items/ActiveItemDefinition.hpp"
#include "active_items/ActiveItemState.hpp"
#include "active_items/ActiveItemUseResult.hpp"
#include "run/RunMapNodeType.hpp"
#include "run/RunState.hpp"

class ActiveItemSystem {
public:
    static int chargeGainForCombatRoom(const ActiveItemDefinition& definition, RunMapNodeType nodeType);
    static int addCharge(RunState& run, const ActiveItemDefinition& definition, int amount);
    static int missingChargeForUse(const ActiveItemState& state, const ActiveItemDefinition& definition);
    static int normalCombatRoomsUntilUsable(const ActiveItemState& state, const ActiveItemDefinition& definition);
    static int addCombatRoomCharge(RunState& run, const ActiveItemDefinition& definition, RunMapNodeType nodeType);
    static bool canUse(const ActiveItemState& state, const ActiveItemDefinition& definition, ActiveItemUseContext context);
    static bool hasEffect(const ActiveItemDefinition& definition, ActiveItemEffectType effectType);
    static bool spendCharge(RunState& run, const ActiveItemDefinition& definition, ActiveItemUseContext context);
    static ActiveItemUseResult use(RunState& run, const ActiveItemDefinition& definition, ActiveItemUseContext context);
    static void equip(RunState& run, const ActiveItemDefinition& definition);
    static void equip(RunState& run, const ActiveItemDefinition& definition, int initialCharge);
};
