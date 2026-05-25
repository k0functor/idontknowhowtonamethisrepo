#include "EffectType.hpp"

#include <stdexcept>

std::string toString(const EffectType& effect) {
    switch (effect) {
        case EffectType::Damage:
            return "damage";
        case EffectType::Block:
            return "block";
        case EffectType::Heal:
            return "heal";
        case EffectType::DrawCards:
            return "draw_cards";
        case EffectType::DiscardCards:
            return "discard_cards";
        case EffectType::ApplyStatus:
            return "apply_status";
        case EffectType::GainEnergy:
            return "gain_energy";
        case EffectType::GainStress:
            return "gain_stress";
        case EffectType::LoseEnergy:
            return "lose_energy";
        case EffectType::LoseStress:
            return "lose_stress";
        case EffectType::LoseHp:
            return "lose_hp";
    }

    throw std::runtime_error("Unknown EffectType");
}

EffectType effectTypeFromString(const std::string_view value) {
    if (value == "damage" || value == "Damage") {
        return EffectType::Damage;
    }

    if (value == "block" || value == "Block") {
        return EffectType::Block;
    }

    if (value == "heal" || value == "Heal") {
        return EffectType::Heal;
    }

    if (value == "draw_cards" || value == "DrawCards") {
        return EffectType::DrawCards;
    }

    if (value == "discard_cards" || value == "DiscardCards") {
        return EffectType::DiscardCards;
    }

    if (value == "apply_status" || value == "ApplyStatus") {
        return EffectType::ApplyStatus;
    }

    if (value == "gain_energy" || value == "GainEnergy") {
        return EffectType::GainEnergy;
    }

    if (value == "gain_stress" || value == "GainStress") {
        return EffectType::GainStress;
    }

    if (value == "lose_energy" || value == "LoseEnergy") {
        return EffectType::LoseEnergy;
    }

    if (value == "lose_stress" || value == "LoseStress") {
        return EffectType::LoseStress;
    }

    if (value == "lose_hp" || value == "LoseHp") {
        return EffectType::LoseHp;
    }

    throw std::runtime_error("Unknown effect type: " + std::string(value));
}
