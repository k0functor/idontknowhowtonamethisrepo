#include "CardPreviewSystem.hpp"

#include "cards/CardUpgrade.hpp"
#include "combat/CardCost.hpp"
#include "combat/EffectContext.hpp"

CardPreviewSystem::CardPreviewSystem(
    const CardDatabase& cardDatabase,
    const CardPlayValidator& validator,
    const EffectResolver& effectResolver,
    const DamageSystem& damageSystem,
    const BlockSystem& blockSystem
)
    : cardDatabase_(cardDatabase),
      validator_(validator),
      effectResolver_(effectResolver),
      damageSystem_(damageSystem),
      blockSystem_(blockSystem) {}

CardPreview CardPreviewSystem::previewCard(
    const CombatState& state,
    const CardInstanceId cardInstanceId,
    const EntityId source,
    const std::optional<EntityId> target
) const {
    const CardInstance& instance = state.hand.get(cardInstanceId);
    const CardDefinition definition = CardUpgrade::effectiveDefinition(cardDatabase_.get(instance.definitionId), instance.upgraded);

    CardPreview preview;
    preview.cardInstanceId = cardInstanceId;
    preview.cardDefinitionId = definition.id;
    preview.energyCost = CardCost::effectiveEnergyCost(state, source, definition);

    const CardPlayValidationResult validation = validator_.validate(
        state,
        definition,
        instance,
        source
    );

    preview.playable = validation.valid;
    preview.unplayableReasonCode = validation.failureReason;
    preview.unplayableReason = validation.reason;

    for (const EffectDefinition& effect : definition.effects) {
        const ResolvedEffectValue resolved = effectResolver_.resolveForPreview(effect.value);

        EffectPreview effectPreview;
        effectPreview.type = effect.type;
        effectPreview.target = effect.target;
        effectPreview.value = PreviewValue{resolved.minimum, resolved.maximum};
        effectPreview.repeatCount = effect.repeatCount;
        effectPreview.statusId = effect.statusId;

        if (effect.type == EffectType::Damage) {
            if (target.has_value()) {
                effectPreview.damage = damageSystem_.previewDamage(
                    state,
                    source,
                    *target,
                    resolved.minimum,
                    resolved.maximum,
                    definition.id,
                    definition.diceCorruption
                );
            } else {
                effectPreview.damage = damageSystem_.previewOutgoingDamage(
                    state,
                    source,
                    resolved.minimum,
                    resolved.maximum,
                    definition.id,
                    definition.diceCorruption
                );
            }
        }

        if (effect.type == EffectType::Block) {
            const ModifiedValueRange blockRange = blockSystem_.previewBlock(
                state,
                source,
                source,
                resolved.minimum,
                resolved.maximum,
                definition.id,
                definition.diceCorruption
            );
            effectPreview.value = PreviewValue{blockRange.modifiedMin, blockRange.modifiedMax};
        }

        preview.effects.push_back(std::move(effectPreview));
    }

    return preview;
}
