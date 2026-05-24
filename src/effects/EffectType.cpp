#include "EffectType.hpp"

#include <stdexcept>

std::string toString(const EffectType& effect) {
    switch (effect)
    {
        case EffectType::Damage:
            return "Damage";
        case EffectType::Block:
            return "Block";
        case EffectType::Heal:
            return "Heal";
        case EffectType::DrawCards:
            return "DrawCards";
        case EffectType::DiscardCards:
            return "DiscardCards";
        case EffectType::ApplyStatus:
            return "ApplyStatus";
        case EffectType::GainEnergy:
            return "GainEnergy";
        case EffectType::GainStress:
            return "GainStress";
        case EffectType::LoseEnergy:
            return "LoseEnergy";
        case EffectType::LoseStress:
            return "LoseStress";
        case EffectType::LoseHp:
            return "LoseHp";
        default:
            throw std::invalid_argument("Invalid EffectType value");
    }
}

EffectType parseEffectType(std::string_view value) {
    if (value == "Damage")
        return EffectType::Damage;
    else if (value == "Block")
        return EffectType::Block;
    else if (value == "Heal")
        return EffectType::Heal;
    else if (value == "DrawCards")
        return EffectType::DrawCards;
    else if (value == "DiscardCards")
        return EffectType::DiscardCards;
    else if (value == "ApplyStatus")
        return EffectType::ApplyStatus;
    else if (value == "GainEnergy")
        return EffectType::GainEnergy;
    else if (value == "GainStress")
        return EffectType::GainStress;
    else if (value == "LoseEnergy")
        return EffectType::LoseEnergy;
    else if (value == "LoseStress")
        return EffectType::LoseStress;
    else if (value == "LoseHp")
        return EffectType::LoseHp;
    else
        throw std::invalid_argument("Invalid string for EffectType: " + std::string(value));
}

