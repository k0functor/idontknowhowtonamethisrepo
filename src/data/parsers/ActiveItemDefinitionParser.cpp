#include "ActiveItemDefinitionParser.hpp"

#include "data/JsonReader.hpp"

#include <stdexcept>

ActiveItemDefinition ActiveItemDefinitionParser::parse(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    ActiveItemDefinition definition;
    definition.id = ActiveItemId(reader.requiredString("id"));
    definition.nameTextId = TextId(reader.requiredString("name"));
    definition.descriptionTextId = TextId(reader.requiredString("description"));
    definition.maxCharge = reader.requiredInt("max_charge");
    definition.chargeCost = reader.requiredInt("charge_cost");
    definition.startingCharge = reader.optionalInt("starting_charge", 0);
    definition.combatCharge = reader.optionalInt("combat_charge", 1);
    definition.eliteCharge = reader.optionalInt("elite_charge", 2);
    definition.bossCharge = reader.optionalInt("boss_charge", 3);
    definition.shopPrice = reader.optionalInt("shop_price", 0);
    definition.canAppearInRewards = reader.optionalBool("reward_eligible", false);
    definition.canAppearInShop = reader.optionalBool("shop_eligible", false);

    if (definition.maxCharge <= 0) {
        throw std::runtime_error("JSON error in '" + sourcePath.string() + "': active item max_charge must be positive");
    }
    if (definition.chargeCost <= 0 || definition.chargeCost > definition.maxCharge) {
        throw std::runtime_error("JSON error in '" + sourcePath.string() + "': active item charge_cost must be in 1..max_charge");
    }
    if (definition.startingCharge < 0 || definition.startingCharge > definition.maxCharge) {
        throw std::runtime_error("JSON error in '" + sourcePath.string() + "': active item starting_charge must be in 0..max_charge");
    }
    if (definition.combatCharge < 0 || definition.eliteCharge < 0 || definition.bossCharge < 0) {
        throw std::runtime_error("JSON error in '" + sourcePath.string() + "': active item room charge values must not be negative");
    }
    if (definition.shopPrice < 0) {
        throw std::runtime_error("JSON error in '" + sourcePath.string() + "': active item shop_price must not be negative");
    }

    const Json& contexts = reader.requiredArray("use_contexts");
    for (const Json& value : contexts) {
        if (!value.is_string()) {
            throw std::runtime_error("JSON error in '" + sourcePath.string() + "': active item use_contexts entries must be strings");
        }
        definition.useContexts.push_back(activeItemUseContextFromString(value.get<std::string>()));
    }
    if (definition.useContexts.empty()) {
        throw std::runtime_error("JSON error in '" + sourcePath.string() + "': active item must have at least one use context");
    }

    const Json& effects = reader.requiredArray("effects");
    for (const Json& effectJson : effects) {
        JsonReader effectReader(effectJson, sourcePath);
        ActiveItemEffectDefinition effect;
        effect.type = activeItemEffectTypeFromString(effectReader.requiredString("type"));
        effect.amount = effectReader.requiredInt("amount");
        if (effect.amount <= 0) {
            throw std::runtime_error("JSON error in '" + sourcePath.string() + "': active item effect amount must be positive");
        }
        definition.effects.push_back(effect);
    }
    if (definition.effects.empty()) {
        throw std::runtime_error("JSON error in '" + sourcePath.string() + "': active item must have at least one effect");
    }

    return definition;
}
