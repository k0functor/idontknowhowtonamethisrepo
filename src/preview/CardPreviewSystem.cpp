#include "CardPreviewSystem.hpp"

#include "cards/CardUpgrade.hpp"
#include "combat/CardCost.hpp"
#include "combat/EffectContext.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace {
bool isAliveEnemy(const CombatState& state, const EntityId id) {
    return state.hasEntity(id) && state.isEnemy(id) && state.entity(id).isAlive();
}

bool isAlivePlayer(const CombatState& state, const EntityId id) {
    return state.hasEntity(id) && state.isPlayer(id) && state.entity(id).isAlive();
}

std::optional<EntityId> firstAliveAllyExceptSource(const CombatState& state, const EntityId source) {
    for (const EntityId candidate : state.alivePlayerIds()) {
        if (candidate != source) {
            return candidate;
        }
    }
    return std::nullopt;
}

std::optional<EntityId> firstAliveEnemy(const CombatState& state) {
    const std::vector<EntityId> enemies = state.aliveEnemyIds();
    if (enemies.empty()) {
        return std::nullopt;
    }
    return enemies.front();
}

std::optional<EntityId> previewTargetForEffect(
    const CombatState& state,
    const EffectTarget effectTarget,
    const EntityId source,
    const std::optional<EntityId> explicitTarget
) {
    switch (effectTarget) {
        case EffectTarget::Self:
            return source;

        case EffectTarget::SingleEnemy:
            if (explicitTarget.has_value() && isAliveEnemy(state, *explicitTarget)) {
                return explicitTarget;
            }
            return firstAliveEnemy(state);

        case EffectTarget::AllEnemies:
        case EffectTarget::RandomEnemy:
            if (explicitTarget.has_value() && isAliveEnemy(state, *explicitTarget)) {
                return explicitTarget;
            }
            return firstAliveEnemy(state);

        case EffectTarget::Ally:
            if (explicitTarget.has_value() && isAlivePlayer(state, *explicitTarget) && *explicitTarget != source) {
                return explicitTarget;
            }
            return firstAliveAllyExceptSource(state, source);

        case EffectTarget::AllAllies:
            if (explicitTarget.has_value() && isAlivePlayer(state, *explicitTarget)) {
                return explicitTarget;
            }
            return source;

        case EffectTarget::RandomAlly:
            if (explicitTarget.has_value() && isAlivePlayer(state, *explicitTarget) && *explicitTarget != source) {
                return explicitTarget;
            }
            return firstAliveAllyExceptSource(state, source);
    }

    return std::nullopt;
}
} // namespace

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
        const std::optional<EntityId> effectTarget = previewTargetForEffect(
            state,
            effect.target,
            source,
            target
        );

        EffectPreview effectPreview;
        effectPreview.type = effect.type;
        effectPreview.target = effect.target;
        effectPreview.value = PreviewValue{resolved.minimum, resolved.maximum};
        effectPreview.repeatCount = effect.repeatCount;
        effectPreview.statusId = effect.statusId;

        if (effect.type == EffectType::Damage) {
            if (effectTarget.has_value() && state.hasEntity(*effectTarget)) {
                effectPreview.damage = damageSystem_.previewDamage(
                    state,
                    source,
                    *effectTarget,
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
            const EntityId blockTarget = effectTarget.value_or(source);
            const ModifiedValueRange blockRange = blockSystem_.previewBlock(
                state,
                source,
                blockTarget,
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
