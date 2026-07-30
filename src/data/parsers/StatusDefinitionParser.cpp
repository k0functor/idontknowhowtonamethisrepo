#include "StatusDefinitionParser.hpp"

#include "data/JsonReader.hpp"

#include <stdexcept>
#include <string>

namespace {
StatusModifierEntity modifierEntityFromString(const std::string& value) {
    if (value == "source") {
        return StatusModifierEntity::Source;
    }
    if (value == "target") {
        return StatusModifierEntity::Target;
    }
    throw std::runtime_error("Unknown status modifier entity: '" + value + "'");
}

StatusModifierOperation modifierOperationFromString(const std::string& value) {
    if (value == "add_per_stack") {
        return StatusModifierOperation::AddPerStack;
    }
    if (value == "add_fixed") {
        return StatusModifierOperation::AddFixed;
    }
    if (value == "multiply_per_stack") {
        return StatusModifierOperation::MultiplyPerStack;
    }
    if (value == "multiply_fixed") {
        return StatusModifierOperation::MultiplyFixed;
    }
    throw std::runtime_error("Unknown status modifier operation: '" + value + "'");
}

StatusTriggerEvent triggerEventFromString(const std::string& value) {
    if (value == "end_owner_turn") {
        return StatusTriggerEvent::EndOwnerTurn;
    }
    throw std::runtime_error("Unknown status trigger event: '" + value + "'");
}

StatusTriggeredEffect triggeredEffectFromString(const std::string& value) {
    if (value == "damage_hp") {
        return StatusTriggeredEffect::DamageHp;
    }
    if (value == "heal") {
        return StatusTriggeredEffect::Heal;
    }
    if (value == "gain_block") {
        return StatusTriggeredEffect::GainBlock;
    }
    throw std::runtime_error("Unknown triggered status effect: '" + value + "'");
}

StatusTriggerLogType triggerLogTypeFromString(const std::string& value) {
    if (value.empty() || value == "none") {
        return StatusTriggerLogType::None;
    }
    if (value == "poison_damage") {
        return StatusTriggerLogType::PoisonDamage;
    }
    if (value == "burn_damage") {
        return StatusTriggerLogType::BurnDamage;
    }
    throw std::runtime_error("Unknown status trigger log type: '" + value + "'");
}

StatusModifierDefinition parseModifier(const Json& json, const std::filesystem::path& sourcePath) {
    JsonReader reader(json, sourcePath);

    StatusModifierDefinition modifier;
    modifier.effectType = effectTypeFromString(reader.requiredString("effect_type"));
    modifier.entity = modifierEntityFromString(reader.optionalString("entity", "source"));
    modifier.operation = modifierOperationFromString(reader.requiredString("operation"));
    modifier.priority = reader.optionalInt("priority", 100);
    modifier.requiresActorStats = reader.optionalBool("requires_actor_stats", true);
    modifier.descriptionTextId = TextId(reader.requiredString("description"));

    switch (modifier.operation) {
        case StatusModifierOperation::AddPerStack:
        case StatusModifierOperation::AddFixed:
            modifier.addAmount = reader.requiredInt("value");
            break;

        case StatusModifierOperation::MultiplyPerStack:
        case StatusModifierOperation::MultiplyFixed:
            modifier.multiplier = reader.requiredDouble("value");
            break;
    }

    return modifier;
}

StatusTriggerDefinition parseTrigger(const Json& json, const std::filesystem::path& sourcePath) {
    JsonReader reader(json, sourcePath);

    StatusTriggerDefinition trigger;
    trigger.event = triggerEventFromString(reader.requiredString("event"));
    trigger.effect = triggeredEffectFromString(reader.requiredString("effect"));
    trigger.flatValue = reader.optionalInt("flat_value", 0);
    trigger.valuePerStack = reader.optionalInt("value_per_stack", 0);
    trigger.removeStacks = reader.optionalInt("remove_stacks", 0);
    trigger.logType = triggerLogTypeFromString(reader.optionalString("log", "none"));
    return trigger;
}
}

StatusDefinition StatusDefinitionParser::parse(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    StatusDefinition definition;
    definition.id = StatusId(reader.requiredString("id"));
    definition.nameTextId = TextId(reader.requiredString("name"));
    definition.descriptionTextId = TextId(reader.requiredString("description"));
    definition.type = statusTypeFromString(reader.optionalString("type", "neutral"));
    definition.durationRule = statusDurationRuleFromString(
        reader.optionalString("duration_rule", "persistent_combat")
    );
    definition.exclusiveGroup = reader.optionalString("exclusive_group", "");

    for (const Json& modifierJson : reader.optionalArray("modifiers")) {
        definition.modifiers.push_back(parseModifier(modifierJson, sourcePath));
    }

    for (const Json& triggerJson : reader.optionalArray("triggers")) {
        definition.triggers.push_back(parseTrigger(triggerJson, sourcePath));
    }

    return definition;
}
