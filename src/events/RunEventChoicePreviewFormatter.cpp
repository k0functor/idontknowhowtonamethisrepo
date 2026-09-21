#include "events/RunEventChoicePreviewFormatter.hpp"

#include "cards/CardId.hpp"
#include "consumables/ConsumableId.hpp"
#include "localization/TextId.hpp"
#include "relics/RelicId.hpp"

#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
using FormatArgs = std::unordered_map<std::string, std::string>;

std::string joinWithComma(const std::vector<std::string>& parts) {
    std::ostringstream out;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            out << ", ";
        }
        out << parts[i];
    }
    return out.str();
}

std::string amountString(const int amount) {
    return std::to_string(amount);
}
}

RunEventChoicePreviewFormatter::RunEventChoicePreviewFormatter(
    const LocalizationManager& localization,
    const CardDatabase& cards,
    const RelicDatabase& relics,
    const ConsumableDatabase& consumables
)
    : localization_(localization), cards_(cards), relics_(relics), consumables_(consumables) {}

std::string RunEventChoicePreviewFormatter::describeChoice(const RunEventChoiceDefinition& choice) const {
    std::vector<std::string> sections;

    const std::string requirements = describeRequirements(choice.requirements);
    if (!requirements.empty()) {
        sections.push_back(requirements);
    }

    const std::string effects = describeEffects(choice.effects);
    if (!effects.empty()) {
        sections.push_back(effects);
    }

    std::ostringstream out;
    for (std::size_t i = 0; i < sections.size(); ++i) {
        if (i > 0) {
            out << '\n';
        }
        out << sections[i];
    }
    return out.str();
}

std::string RunEventChoicePreviewFormatter::describeEffects(const std::vector<RunEventEffect>& effects) const {
    std::vector<std::string> parts;
    for (const RunEventEffect& effect : effects) {
        std::string text = effectText(effect);
        if (!text.empty()) {
            parts.push_back(std::move(text));
        }
    }

    if (parts.empty()) {
        return {};
    }

    return localization_.format(
        TextId("event.choice.preview.effects"),
        FormatArgs{{"effects", joinWithComma(parts)}}
    );
}

std::string RunEventChoicePreviewFormatter::describeRequirements(const RunEventChoiceRequirements& requirements) const {
    std::vector<std::string> parts;

    if (requirements.minGold > 0) {
        parts.push_back(localization_.format(
            TextId("event.choice.requirement.min_gold"),
            FormatArgs{{"required", amountString(requirements.minGold)}}
        ));
    }

    if (requirements.minHp > 0) {
        parts.push_back(localization_.format(
            TextId("event.choice.requirement.min_hp"),
            FormatArgs{{"required", amountString(requirements.minHp)}}
        ));
    }

    if (requirements.freeConsumableSlot) {
        parts.push_back(localization_.get(TextId("event.choice.requirement.free_consumable_slot")));
    }

    if (requirements.minDeckSize > 0) {
        parts.push_back(localization_.format(
            TextId("event.choice.requirement.min_deck_size"),
            FormatArgs{{"required", amountString(requirements.minDeckSize)}}
        ));
    }

    if (requirements.maxDeckSize > 0) {
        parts.push_back(localization_.format(
            TextId("event.choice.requirement.max_deck_size"),
            FormatArgs{{"required", amountString(requirements.maxDeckSize)}}
        ));
    }

    if (requirements.minMissingHp > 0) {
        parts.push_back(localization_.format(
            TextId("event.choice.requirement.min_missing_hp"),
            FormatArgs{{"required", amountString(requirements.minMissingHp)}}
        ));
    }

    if (requirements.minUpgradedCards > 0) {
        parts.push_back(localization_.format(
            TextId("event.choice.requirement.min_upgraded_cards"),
            FormatArgs{{"required", amountString(requirements.minUpgradedCards)}}
        ));
    }

    if (!requirements.requiredMechanicId.empty()) {
        parts.push_back(localization_.get(TextId("event.choice.requirement.run_mechanic")));
    }

    if (requirements.minStress > 0) {
        parts.push_back(localization_.format(
            TextId("event.choice.requirement.min_stress"),
            FormatArgs{{"required", amountString(requirements.minStress)}}
        ));
    }

    if (requirements.maxStress > 0) {
        parts.push_back(localization_.format(
            TextId("event.choice.requirement.max_stress"),
            FormatArgs{{"required", amountString(requirements.maxStress)}}
        ));
    }

    for (const std::string& cardId : requirements.requiredCardIds) {
        parts.push_back(localization_.format(
            TextId("event.choice.requirement.required_card"),
            FormatArgs{{"name", cardName(cardId)}}
        ));
    }

    for (const std::string& cardId : requirements.forbiddenCardIds) {
        parts.push_back(localization_.format(
            TextId("event.choice.requirement.forbidden_card"),
            FormatArgs{{"name", cardName(cardId)}}
        ));
    }

    for (const std::string& relicId : requirements.requiredRelicIds) {
        parts.push_back(localization_.format(
            TextId("event.choice.requirement.required_relic"),
            FormatArgs{{"name", relicName(relicId)}}
        ));
    }

    for (const std::string& relicId : requirements.forbiddenRelicIds) {
        parts.push_back(localization_.format(
            TextId("event.choice.requirement.forbidden_relic"),
            FormatArgs{{"name", relicName(relicId)}}
        ));
    }

    for (const std::string& traitId : requirements.requiredTraitIds) {
        parts.push_back(localization_.format(
            TextId("event.choice.requirement.required_trait"),
            FormatArgs{{"name", traitName(traitId)}}
        ));
    }

    for (const std::string& traitId : requirements.forbiddenTraitIds) {
        parts.push_back(localization_.format(
            TextId("event.choice.requirement.forbidden_trait"),
            FormatArgs{{"name", traitName(traitId)}}
        ));
    }

    for (const std::string& flag : requirements.requiredEventFlags) {
        (void)flag;
        parts.push_back(localization_.get(TextId("event.choice.requirement.required_event_flag")));
    }

    for (const std::string& flag : requirements.forbiddenEventFlags) {
        (void)flag;
        parts.push_back(localization_.get(TextId("event.choice.requirement.forbidden_event_flag")));
    }

    if (parts.empty()) {
        return {};
    }

    return localization_.format(
        TextId("event.choice.preview.requirements"),
        FormatArgs{{"requirements", joinWithComma(parts)}}
    );
}

std::string RunEventChoicePreviewFormatter::describeBlockReason(const RunEventChoiceBlockReason& reason) const {
    switch (reason.type) {
        case RunEventChoiceBlockReasonType::NotEnoughGold:
            return localization_.format(
                TextId("event.choice.unavailable.gold"),
                FormatArgs{{"current", amountString(reason.current)}, {"required", amountString(reason.required)}}
            );

        case RunEventChoiceBlockReasonType::NotEnoughHp:
            return localization_.format(
                TextId("event.choice.unavailable.hp"),
                FormatArgs{{"current", amountString(reason.current)}, {"required", amountString(reason.required)}}
            );

        case RunEventChoiceBlockReasonType::NoFreeConsumableSlot:
            return localization_.format(
                TextId("event.choice.unavailable.consumable_slot"),
                FormatArgs{{"current", amountString(reason.current)}, {"maximum", amountString(reason.required)}}
            );

        case RunEventChoiceBlockReasonType::NotEnoughCards:
            return localization_.format(
                TextId("event.choice.unavailable.deck_size"),
                FormatArgs{{"current", amountString(reason.current)}, {"required", amountString(reason.required)}}
            );

        case RunEventChoiceBlockReasonType::TooManyCards:
            return localization_.format(
                TextId("event.choice.unavailable.deck_size_max"),
                FormatArgs{{"current", amountString(reason.current)}, {"required", amountString(reason.required)}}
            );

        case RunEventChoiceBlockReasonType::NotWoundedEnough:
            return localization_.format(
                TextId("event.choice.unavailable.missing_hp"),
                FormatArgs{{"current", amountString(reason.current)}, {"required", amountString(reason.required)}}
            );

        case RunEventChoiceBlockReasonType::NotEnoughUpgradedCards:
            return localization_.format(
                TextId("event.choice.unavailable.upgraded_cards"),
                FormatArgs{{"current", amountString(reason.current)}, {"required", amountString(reason.required)}}
            );

        case RunEventChoiceBlockReasonType::MissingRequiredRelic:
            return localization_.format(
                TextId("event.choice.unavailable.required_relic"),
                FormatArgs{{"name", relicName(reason.id)}}
            );

        case RunEventChoiceBlockReasonType::HasForbiddenRelic:
            return localization_.format(
                TextId("event.choice.unavailable.forbidden_relic"),
                FormatArgs{{"name", relicName(reason.id)}}
            );

        case RunEventChoiceBlockReasonType::MissingRequiredCard:
            return localization_.format(
                TextId("event.choice.unavailable.required_card"),
                FormatArgs{{"name", cardName(reason.id)}}
            );

        case RunEventChoiceBlockReasonType::HasForbiddenCard:
            return localization_.format(
                TextId("event.choice.unavailable.forbidden_card"),
                FormatArgs{{"name", cardName(reason.id)}}
            );

        case RunEventChoiceBlockReasonType::WrongRunMechanic:
            return localization_.get(TextId("event.choice.unavailable.run_mechanic"));

        case RunEventChoiceBlockReasonType::StressTooLow:
            return localization_.format(
                TextId("event.choice.unavailable.stress_low"),
                FormatArgs{{"current", amountString(reason.current)}, {"required", amountString(reason.required)}}
            );

        case RunEventChoiceBlockReasonType::StressTooHigh:
            return localization_.format(
                TextId("event.choice.unavailable.stress_high"),
                FormatArgs{{"current", amountString(reason.current)}, {"required", amountString(reason.required)}}
            );

        case RunEventChoiceBlockReasonType::MissingRequiredTrait:
            return localization_.format(
                TextId("event.choice.unavailable.required_trait"),
                FormatArgs{{"name", traitName(reason.id)}}
            );

        case RunEventChoiceBlockReasonType::HasForbiddenTrait:
            return localization_.format(
                TextId("event.choice.unavailable.forbidden_trait"),
                FormatArgs{{"name", traitName(reason.id)}}
            );

        case RunEventChoiceBlockReasonType::MissingRequiredEventFlag:
            return localization_.get(TextId("event.choice.unavailable.required_event_flag"));

        case RunEventChoiceBlockReasonType::HasForbiddenEventFlag:
            return localization_.get(TextId("event.choice.unavailable.forbidden_event_flag"));
    }

    return localization_.get(TextId("event.choice.unavailable.unknown"));
}

std::string RunEventChoicePreviewFormatter::effectText(const RunEventEffect& effect) const {
    switch (effect.type) {
        case RunEventEffectType::GainGold:
            return localization_.format(TextId("event.choice.effect.gain_gold"), FormatArgs{{"amount", amountString(effect.amount)}});
        case RunEventEffectType::LoseGold:
            return localization_.format(TextId("event.choice.effect.lose_gold"), FormatArgs{{"amount", amountString(effect.amount)}});
        case RunEventEffectType::GainCard:
            return localization_.format(TextId("event.choice.effect.gain_card"), FormatArgs{{"name", cardName(effect.contentId)}});
        case RunEventEffectType::GainRandomCard:
            return localization_.get(TextId("event.choice.effect.gain_random_card"));
        case RunEventEffectType::GainRelic:
            return localization_.format(TextId("event.choice.effect.gain_relic"), FormatArgs{{"name", relicName(effect.contentId)}});
        case RunEventEffectType::GainRandomRelic:
            return localization_.get(TextId("event.choice.effect.gain_random_relic"));
        case RunEventEffectType::GainConsumable:
            return localization_.format(TextId("event.choice.effect.gain_consumable"), FormatArgs{{"name", consumableName(effect.contentId)}});
        case RunEventEffectType::GainRandomConsumable:
            return localization_.get(TextId("event.choice.effect.gain_random_consumable"));
        case RunEventEffectType::RemoveCard:
            return localization_.format(TextId("event.choice.effect.remove_card"), FormatArgs{{"name", cardName(effect.contentId)}});
        case RunEventEffectType::RemoveRandomCard:
            return localization_.get(TextId("event.choice.effect.remove_random_card"));
        case RunEventEffectType::UpgradeRandomCard:
            return localization_.get(TextId("event.choice.effect.upgrade_random_card"));
        case RunEventEffectType::GainStress:
            return localization_.format(TextId("event.choice.effect.gain_stress"), FormatArgs{{"amount", amountString(effect.amount)}});
        case RunEventEffectType::LoseStress:
            return localization_.format(TextId("event.choice.effect.lose_stress"), FormatArgs{{"amount", amountString(effect.amount)}});
        case RunEventEffectType::LoseHp:
            return localization_.format(TextId("event.choice.effect.lose_hp"), FormatArgs{{"amount", amountString(effect.amount)}});
        case RunEventEffectType::HealAll:
            return effect.amount > 0
                ? localization_.format(TextId("event.choice.effect.heal_all_amount"), FormatArgs{{"amount", amountString(effect.amount)}})
                : localization_.get(TextId("event.choice.effect.heal_all"));
        case RunEventEffectType::SetFlag:
        case RunEventEffectType::ClearFlag:
            return {};
        case RunEventEffectType::Skip:
            return localization_.get(TextId("event.choice.effect.skip"));
    }

    return {};
}

std::string RunEventChoicePreviewFormatter::cardName(const std::string& cardId) const {
    const CardId id(cardId);
    if (!cards_.contains(id)) {
        return cardId;
    }
    return localization_.get(cards_.get(id).nameTextId);
}

std::string RunEventChoicePreviewFormatter::relicName(const std::string& relicId) const {
    const RelicId id(relicId);
    if (!relics_.contains(id)) {
        return relicId;
    }
    return localization_.get(relics_.get(id).nameTextId);
}

std::string RunEventChoicePreviewFormatter::consumableName(const std::string& consumableId) const {
    const ConsumableId id(consumableId);
    if (!consumables_.contains(id)) {
        return consumableId;
    }
    return localization_.get(consumables_.get(id).nameTextId);
}


std::string RunEventChoicePreviewFormatter::traitName(const std::string& traitId) const {
    return localization_.get(TextId("trait." + traitId + ".name"));
}
