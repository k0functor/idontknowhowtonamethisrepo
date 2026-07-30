#include "CardViewModelBuilder.hpp"

#include "cards/CardDefinition.hpp"
#include "cards/CardDescriptionFormatter.hpp"
#include "cards/CardUpgrade.hpp"

#include <algorithm>

namespace {
std::string localizedOrFallback(
    const LocalizationManager& localization,
    const TextId& textId,
    const std::string&
) {
    return localization.get(textId);
}

std::string formatOrFallback(
    const LocalizationManager& localization,
    const TextId& textId,
    const TextFormatter::Variables& variables,
    const std::string&
) {
    return localization.format(textId, variables);
}

std::string entityDisplayName(
    const CombatState& state,
    const LocalizationManager& localization,
    const EntityId entityId,
    const std::string& fallback
) {
    if (!state.hasEntity(entityId)) {
        return fallback;
    }

    const CombatEntity& entity = state.entity(entityId);
    return localizedOrFallback(localization, entity.nameTextId, entity.definitionId.empty() ? fallback : entity.definitionId);
}

std::string sourceOwnerLabel(
    const CombatState& state,
    const LocalizationManager& localization,
    const CardDefinition& definition,
    const EntityId source
) {
    if (definition.ownerActorId.empty()) {
        const std::string actor = entityDisplayName(state, localization, source, "actor");
        return formatOrFallback(
            localization,
            TextId("ui.card_owner.shared_source"),
            {{"actor", actor}},
            {}
        );
    }

    return formatOrFallback(
        localization,
        TextId("ui.card_owner.actor"),
        {{"actor", entityDisplayName(state, localization, source, definition.ownerActorId)}},
        {}
    );
}

std::string cardFailureReasonText(
    const CombatState& state,
    const LocalizationManager& localization,
    const CardPlayFailureReason reason,
    const EntityId source,
    const int energyCost,
    const int stressCost,
    const std::string& fallback
) {
    switch (reason) {
        case CardPlayFailureReason::None:
            return {};
        case CardPlayFailureReason::NoPlayerActor:
            return localizedOrFallback(localization, TextId("ui.card_unplayable.no_player_actor"), "No player actor");
        case CardPlayFailureReason::NotPlayerTurn:
            return localizedOrFallback(localization, TextId("ui.card_unplayable.not_player_turn"), "Not player turn");
        case CardPlayFailureReason::CardNotInHand:
            return localizedOrFallback(localization, TextId("ui.card_unplayable.card_not_in_hand"), "Card is not in hand");
        case CardPlayFailureReason::InvalidCardSource:
            return localizedOrFallback(localization, TextId("ui.card_unplayable.invalid_source"), "Invalid card source");
        case CardPlayFailureReason::CardSourceDefeated:
            return formatOrFallback(
                localization,
                TextId("ui.card_unplayable.source_defeated"),
                {{"actor", entityDisplayName(state, localization, source, "actor")}},
                {}
            );
        case CardPlayFailureReason::WrongActorTurn: {
            const std::optional<EntityId> activePlayer = state.activePlayerId();
            if (activePlayer.has_value()) {
                const std::string activeName = entityDisplayName(state, localization, *activePlayer, "actor");
                return formatOrFallback(
                    localization,
                    TextId("ui.card_unplayable.wrong_actor_turn"),
                    {{"actor", activeName}},
                    {}
                );
            }

            return localizedOrFallback(localization, TextId("ui.card_unplayable.wrong_actor_turn_no_active"), "Not this actor's turn");
        }
        case CardPlayFailureReason::WrongActorForCard:
            return formatOrFallback(
                localization,
                TextId("ui.card_unplayable.wrong_actor_for_card"),
                {{"actor", entityDisplayName(state, localization, source, "actor")}},
                {}
            );
        case CardPlayFailureReason::NotEnoughEnergy:
            return formatOrFallback(
                localization,
                TextId("ui.card_unplayable.not_enough_energy_actor"),
                {
                    {"actor", entityDisplayName(state, localization, source, "actor")},
                    {"current", std::to_string(state.resources.energyFor(source))},
                    {"cost", std::to_string(energyCost)}
                },
                {}
            );
        case CardPlayFailureReason::NotEnoughStress:
            return formatOrFallback(
                localization,
                TextId("ui.card_unplayable.not_enough_stress_actor"),
                {
                    {"actor", entityDisplayName(state, localization, source, "actor")},
                    {"current", state.hasEntity(source) ? std::to_string(state.entity(source).stress) : "0"},
                    {"cost", std::to_string(stressCost)}
                },
                {}
            );
        case CardPlayFailureReason::UnplayableKeyword:
            return localizedOrFallback(localization, TextId("ui.card_unplayable.unplayable_keyword"), "Unplayable");
    }

    return fallback;
}
}

CardViewModelBuilder::CardViewModelBuilder(
    const CardDatabase& cardDatabase,
    const LocalizationManager& localization,
    const CardPreviewSystem& previewSystem
)
    : cardDatabase_(cardDatabase),
      localization_(localization),
      previewSystem_(previewSystem) {}

CardViewModel CardViewModelBuilder::build(
    const CombatState& state,
    const CardInstanceId cardInstanceId,
    const EntityId source,
    const std::optional<EntityId> target
) const {
    const CardInstance& instance = state.hand.get(cardInstanceId);
    const CardDefinition definition = CardUpgrade::effectiveDefinition(cardDatabase_.get(instance.definitionId), instance.upgraded);

    const CardPreview preview = previewSystem_.previewCard(
        state,
        cardInstanceId,
        source,
        target
    );

    const CardDescriptionFormatter descriptionFormatter(localization_);

    CardViewModel model;
    model.instanceId = cardInstanceId;
    model.name = localization_.get(definition.nameTextId) + (instance.upgraded ? "+" : "");
    model.description = descriptionFormatter.formatCombatDescription(
        definition,
        preview,
        state,
        source,
        target
    );
    model.ownerLabel = sourceOwnerLabel(state, localization_, definition, source);
    model.sourceActorName = entityDisplayName(state, localization_, source, "actor");
    model.sourceCurrentEnergy = state.resources.energyFor(source);
    model.sourceMaxEnergy = state.resources.maxEnergyFor(source);
    model.sharedSource = definition.ownerActorId.empty();
    model.sourceCanPay = state.resources.canSpendEnergy(source, preview.energyCost);
    if (state.players.size() > 1u) {
        model.sourceEnergyLabel = formatOrFallback(
            localization_,
            TextId("ui.card_source.energy"),
            {
                {"current", std::to_string(model.sourceCurrentEnergy)},
                {"max", std::to_string(model.sourceMaxEnergy)},
                {"cost", std::to_string(preview.energyCost)}
            },
            {}
        );
    }
    model.energyCost = preview.energyCost;
    model.type = definition.type;
    model.rarity = definition.rarity;
    model.upgraded = instance.upgraded;
    model.playable = preview.playable;
    int requiredStress = 0;
    for (const EffectPreview& effect : preview.effects) {
        requiredStress += effect.stressCost * std::max(1, effect.repeatCount);
    }

    model.unplayableReason = cardFailureReasonText(
        state,
        localization_,
        preview.unplayableReasonCode,
        source,
        preview.energyCost,
        requiredStress,
        preview.unplayableReason
    );
    return model;
}
