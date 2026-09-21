#include "ActiveItemSystem.hpp"

#include <algorithm>

int ActiveItemSystem::chargeGainForCombatRoom(
    const ActiveItemDefinition& definition,
    const RunMapNodeType nodeType
) {
    switch (nodeType) {
        case RunMapNodeType::Combat: return definition.combatCharge;
        case RunMapNodeType::Elite: return definition.eliteCharge;
        case RunMapNodeType::Boss: return definition.bossCharge;
        case RunMapNodeType::Event:
        case RunMapNodeType::Shop:
        case RunMapNodeType::Chest:
        case RunMapNodeType::Rest:
            return 0;
    }
    return 0;
}

int ActiveItemSystem::addCharge(
    RunState& run,
    const ActiveItemDefinition& definition,
    const int amount
) {
    if (run.activeItem.itemId != definition.id.value || amount <= 0) {
        return 0;
    }

    const int before = std::clamp(run.activeItem.charge, 0, definition.maxCharge);
    run.activeItem.charge = std::clamp(before + amount, 0, definition.maxCharge);
    const int actualGain = run.activeItem.charge - before;
    run.stats.activeItemChargeGained += actualGain;
    return actualGain;
}

int ActiveItemSystem::missingChargeForUse(
    const ActiveItemState& state,
    const ActiveItemDefinition& definition
) {
    if (state.itemId != definition.id.value) {
        return definition.chargeCost;
    }
    return std::max(0, definition.chargeCost - std::max(0, state.charge));
}

int ActiveItemSystem::normalCombatRoomsUntilUsable(
    const ActiveItemState& state,
    const ActiveItemDefinition& definition
) {
    const int missing = missingChargeForUse(state, definition);
    if (missing <= 0) {
        return 0;
    }
    if (definition.combatCharge <= 0) {
        return -1;
    }
    return (missing + definition.combatCharge - 1) / definition.combatCharge;
}

int ActiveItemSystem::addCombatRoomCharge(
    RunState& run,
    const ActiveItemDefinition& definition,
    const RunMapNodeType nodeType
) {
    return addCharge(run, definition, chargeGainForCombatRoom(definition, nodeType));
}

bool ActiveItemSystem::canUse(
    const ActiveItemState& state,
    const ActiveItemDefinition& definition,
    const ActiveItemUseContext context
) {
    if (state.itemId != definition.id.value || state.charge < definition.chargeCost) return false;
    return std::find(definition.useContexts.begin(), definition.useContexts.end(), context) != definition.useContexts.end();
}


bool ActiveItemSystem::hasEffect(
    const ActiveItemDefinition& definition,
    const ActiveItemEffectType effectType
) {
    return std::any_of(
        definition.effects.begin(),
        definition.effects.end(),
        [effectType](const ActiveItemEffectDefinition& effect) { return effect.type == effectType; }
    );
}

bool ActiveItemSystem::spendCharge(
    RunState& run,
    const ActiveItemDefinition& definition,
    const ActiveItemUseContext context
) {
    if (!canUse(run.activeItem, definition, context)) {
        return false;
    }

    run.activeItem.charge -= definition.chargeCost;
    ++run.stats.activeItemsUsed;
    return true;
}

ActiveItemUseResult ActiveItemSystem::use(
    RunState& run,
    const ActiveItemDefinition& definition,
    const ActiveItemUseContext context
) {
    ActiveItemUseResult result;
    result.itemId = run.activeItem.itemId;
    if (run.activeItem.empty()) {
        result.status = ActiveItemUseStatus::NoItem;
        return result;
    }
    if (run.activeItem.itemId != definition.id.value) {
        result.status = ActiveItemUseStatus::UnknownItem;
        return result;
    }
    if (std::find(definition.useContexts.begin(), definition.useContexts.end(), context) == definition.useContexts.end()) {
        result.status = ActiveItemUseStatus::InvalidContext;
        return result;
    }
    if (run.activeItem.charge < definition.chargeCost) {
        result.status = ActiveItemUseStatus::NotEnoughCharge;
        return result;
    }

    for (const ActiveItemEffectDefinition& effect : definition.effects) {
        switch (effect.type) {
            case ActiveItemEffectType::HealParty:
                for (RunActorState& actor : run.actorStates) {
                    if (actor.currentHp <= 0) continue;
                    const int before = actor.currentHp;
                    actor.currentHp = std::min(actor.maxHp, actor.currentHp + effect.amount);
                    result.healed += actor.currentHp - before;
                }
                break;
            case ActiveItemEffectType::GainGold:
                run.gold += effect.amount;
                run.stats.goldGained += effect.amount;
                result.goldGained += effect.amount;
                break;
            case ActiveItemEffectType::RerollOffers:
            case ActiveItemEffectType::SkipEnemyTurn:
            case ActiveItemEffectType::CreateConsumable:
            case ActiveItemEffectType::RerollMapChoices:
            case ActiveItemEffectType::CopyCard:
            case ActiveItemEffectType::StabilizeStress:
                break;
        }
    }

    if (result.healed <= 0 && result.goldGained <= 0) {
        result.status = ActiveItemUseStatus::NoEffect;
        return result;
    }

    if (!spendCharge(run, definition, context)) {
        result.status = ActiveItemUseStatus::NotEnoughCharge;
        return result;
    }
    result.chargeSpent = definition.chargeCost;
    result.status = ActiveItemUseStatus::Used;
    return result;
}

void ActiveItemSystem::equip(
    RunState& run,
    const ActiveItemDefinition& definition
) {
    equip(run, definition, definition.startingCharge);
}

void ActiveItemSystem::equip(
    RunState& run,
    const ActiveItemDefinition& definition,
    const int initialCharge
) {
    run.activeItem.itemId = definition.id.value;
    run.activeItem.charge = std::clamp(initialCharge, 0, definition.maxCharge);
}
