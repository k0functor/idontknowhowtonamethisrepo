#pragma once

#include "effects/EffectType.hpp"
#include "localization/TextId.hpp"
#include "statuses/StatusDurationRule.hpp"
#include "statuses/StatusId.hpp"
#include "statuses/StatusType.hpp"

#include <string>
#include <vector>

enum class StatusModifierEntity {
    Source,
    Target
};

enum class StatusModifierOperation {
    AddPerStack,
    AddFixed,
    MultiplyPerStack,
    MultiplyFixed
};

struct StatusModifierDefinition {
    EffectType effectType = EffectType::Damage;
    StatusModifierEntity entity = StatusModifierEntity::Source;
    StatusModifierOperation operation = StatusModifierOperation::AddPerStack;

    int addAmount = 0;
    double multiplier = 1.0;

    int priority = 100;
    bool requiresActorStats = true;
    TextId descriptionTextId;
};

enum class StatusTriggerEvent {
    EndOwnerTurn
};

enum class StatusTriggeredEffect {
    DamageHp,
    Heal,
    GainBlock
};

enum class StatusTriggerLogType {
    None,
    PoisonDamage,
    BurnDamage
};

struct StatusTriggerDefinition {
    StatusTriggerEvent event = StatusTriggerEvent::EndOwnerTurn;
    StatusTriggeredEffect effect = StatusTriggeredEffect::DamageHp;

    int flatValue = 0;
    int valuePerStack = 0;
    int removeStacks = 0;

    StatusTriggerLogType logType = StatusTriggerLogType::None;
};

struct StatusDefinition {
    StatusId id;

    TextId nameTextId;
    TextId descriptionTextId;

    StatusType type = StatusType::Neutral;
    StatusDurationRule durationRule = StatusDurationRule::PersistentCombat;

    std::string exclusiveGroup;
    std::vector<StatusModifierDefinition> modifiers;
    std::vector<StatusTriggerDefinition> triggers;
};
