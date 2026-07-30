#include "ActiveItemEffectType.hpp"

#include <stdexcept>

ActiveItemEffectType activeItemEffectTypeFromString(const std::string& value) {
    if (value == "heal_party") return ActiveItemEffectType::HealParty;
    if (value == "gain_gold") return ActiveItemEffectType::GainGold;
    if (value == "reroll_offers") return ActiveItemEffectType::RerollOffers;
    if (value == "skip_enemy_turn") return ActiveItemEffectType::SkipEnemyTurn;
    if (value == "create_consumable") return ActiveItemEffectType::CreateConsumable;
    if (value == "reroll_map_choices") return ActiveItemEffectType::RerollMapChoices;
    if (value == "copy_card") return ActiveItemEffectType::CopyCard;
    if (value == "stabilize_stress") return ActiveItemEffectType::StabilizeStress;
    throw std::runtime_error("Unknown active item effect type: " + value);
}

std::string toString(const ActiveItemEffectType type) {
    switch (type) {
        case ActiveItemEffectType::HealParty: return "heal_party";
        case ActiveItemEffectType::GainGold: return "gain_gold";
        case ActiveItemEffectType::RerollOffers: return "reroll_offers";
        case ActiveItemEffectType::SkipEnemyTurn: return "skip_enemy_turn";
        case ActiveItemEffectType::CreateConsumable: return "create_consumable";
        case ActiveItemEffectType::RerollMapChoices: return "reroll_map_choices";
        case ActiveItemEffectType::CopyCard: return "copy_card";
        case ActiveItemEffectType::StabilizeStress: return "stabilize_stress";
    }
    return "heal_party";
}
