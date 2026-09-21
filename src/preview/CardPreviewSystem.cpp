#include "CardPreviewSystem.hpp"

#include "cards/CardUpgrade.hpp"
#include "combat/CardCost.hpp"
#include "combat/CardStressCost.hpp"
#include "combat/EffectContext.hpp"
#include "combat/EffectScaling.hpp"

#include <algorithm>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
struct PreviewTargets {
    std::vector<EntityId> candidates;
    bool requiresSelection = false;
    bool random = false;
};

bool isAliveEnemy(const CombatState& state, const EntityId id) {
    return state.hasEntity(id) && state.isEnemy(id) && state.entity(id).isAlive();
}

bool isAlivePlayer(const CombatState& state, const EntityId id) {
    return state.hasEntity(id) && state.isPlayer(id) && state.entity(id).isAlive();
}

std::vector<EntityId> aliveAlliesExceptSource(const CombatState& state, const EntityId source) {
    std::vector<EntityId> result;
    for (const EntityId candidate : state.alivePlayerIds()) {
        if (candidate != source) {
            result.push_back(candidate);
        }
    }
    return result;
}

PreviewTargets previewTargetsForEffect(
    const CombatState& state,
    const EffectTarget effectTarget,
    const EntityId source,
    const std::optional<EntityId> explicitTarget
) {
    PreviewTargets result;

    switch (effectTarget) {
        case EffectTarget::Self:
            result.candidates.push_back(source);
            return result;

        case EffectTarget::SingleEnemy: {
            if (explicitTarget.has_value() && isAliveEnemy(state, *explicitTarget)) {
                result.candidates.push_back(*explicitTarget);
                return result;
            }

            result.candidates = state.aliveEnemyIds();
            if (result.candidates.size() == 1u) {
                return result;
            }

            result.candidates.clear();
            result.requiresSelection = !state.aliveEnemyIds().empty();
            return result;
        }

        case EffectTarget::AllEnemies:
            result.candidates = state.aliveEnemyIds();
            return result;

        case EffectTarget::RandomEnemy:
            result.candidates = state.aliveEnemyIds();
            result.random = true;
            return result;

        case EffectTarget::Ally: {
            if (explicitTarget.has_value() && isAlivePlayer(state, *explicitTarget) && *explicitTarget != source) {
                result.candidates.push_back(*explicitTarget);
                return result;
            }

            result.candidates = aliveAlliesExceptSource(state, source);
            if (result.candidates.size() == 1u) {
                return result;
            }

            result.requiresSelection = result.candidates.size() > 1u;
            result.candidates.clear();
            return result;
        }

        case EffectTarget::AllAllies:
            result.candidates = state.alivePlayerIds();
            return result;

        case EffectTarget::RandomAlly:
            result.candidates = aliveAlliesExceptSource(state, source);
            result.random = true;
            return result;
    }

    return result;
}

void appendUnique(std::vector<std::string>& output, const std::vector<std::string>& values) {
    for (const std::string& value : values) {
        if (!value.empty() && std::find(output.begin(), output.end(), value) == output.end()) {
            output.push_back(value);
        }
    }
}

void addRange(PreviewValue& target, const int minimum, const int maximum) {
    target.minimum += minimum;
    target.maximum += maximum;
}

PreviewValue randomRange(const std::vector<PreviewValue>& values) {
    if (values.empty()) {
        return {};
    }

    PreviewValue result{
        std::numeric_limits<int>::max(),
        std::numeric_limits<int>::min()
    };
    for (const PreviewValue& value : values) {
        result.minimum = std::min(result.minimum, value.minimum);
        result.maximum = std::max(result.maximum, value.maximum);
    }
    return result;
}

struct PreviewBlockState {
    int minimum = 0;
    int maximum = 0;
};

using PreviewBlockStates = std::unordered_map<std::uint64_t, PreviewBlockState>;

PreviewBlockState& blockStateFor(
    PreviewBlockStates& states,
    const CombatState& state,
    const EntityId target
) {
    const auto [iterator, inserted] = states.try_emplace(target.value);
    if (inserted && state.hasEntity(target)) {
        iterator->second.minimum = state.entity(target).block;
        iterator->second.maximum = state.entity(target).block;
    }
    return iterator->second;
}

DamagePreview applyRepeatedDamagePreview(
    const DamagePreview& perHit,
    const int repetitions,
    PreviewBlockState& blockState
) {
    DamagePreview total = perHit;
    total.rawMin *= repetitions;
    total.rawMax *= repetitions;
    total.modifiedMin *= repetitions;
    total.modifiedMax *= repetitions;
    total.blockedMin = 0;
    total.blockedMax = 0;
    total.hpDamageMin = 0;
    total.hpDamageMax = 0;

    for (int hit = 0; hit < repetitions; ++hit) {
        const int blockedOnMinimumPath = std::min(blockState.maximum, perHit.modifiedMin);
        total.blockedMin += blockedOnMinimumPath;
        total.hpDamageMin += std::max(0, perHit.modifiedMin - blockedOnMinimumPath);
        blockState.maximum -= blockedOnMinimumPath;

        const int blockedOnMaximumPath = std::min(blockState.minimum, perHit.modifiedMax);
        total.blockedMax += blockedOnMaximumPath;
        total.hpDamageMax += std::max(0, perHit.modifiedMax - blockedOnMaximumPath);
        blockState.minimum -= blockedOnMaximumPath;
    }

    return total;
}

int targetCountForSummary(const EffectTarget target, const PreviewTargets& targets) {
    if (target == EffectTarget::SingleEnemy || target == EffectTarget::Ally ||
        target == EffectTarget::RandomEnemy || target == EffectTarget::RandomAlly) {
        return targets.candidates.empty() && !targets.requiresSelection ? 0 : 1;
    }
    return static_cast<int>(targets.candidates.size());
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
    preview.stressCost = CardStressCost::totalCost(definition);
    if (state.hasEntity(source)) {
        const CombatEntity& sourceEntity = state.entity(source);
        preview.sourceStressBefore = sourceEntity.stress;
        preview.sourceMaxStress = sourceEntity.maxStress;
    }

    const CardPlayValidationResult validation = validator_.validate(
        state,
        definition,
        instance,
        source
    );

    preview.playable = validation.valid;
    preview.unplayableReasonCode = validation.failureReason;
    preview.unplayableReason = validation.reason;

    int cumulativeStressCost = CardStressCost::directCost(definition);
    int sourceStressDeltaMinimum = -CardStressCost::directCost(definition);
    int sourceStressDeltaMaximum = -CardStressCost::directCost(definition);

    PreviewBlockStates previewBlockStates;

    for (const EffectDefinition& effect : definition.effects) {
        const ResolvedEffectValue resolved = effectResolver_.resolveForPreview(effect.value);
        const PreviewTargets targets = previewTargetsForEffect(state, effect.target, source, target);
        const std::optional<EntityId> representativeTarget = targets.candidates.empty()
            ? std::nullopt
            : std::optional<EntityId>(targets.candidates.front());

        const int representativeScalingBonus = effectScalingBonus(state, effect, source, representativeTarget);
        const int scaledMinimum = std::max(0, resolved.minimum + representativeScalingBonus);
        const int scaledMaximum = std::max(0, resolved.maximum + representativeScalingBonus);
        const int repetitions = std::max(1, effect.repeatCount);

        EffectPreview effectPreview;
        effectPreview.type = effect.type;
        effectPreview.target = effect.target;
        effectPreview.value = PreviewValue{scaledMinimum, scaledMaximum};
        effectPreview.repeatCount = effect.repeatCount;
        effectPreview.statusId = effect.statusId;

        preview.outcome.requiresTargetSelection =
            preview.outcome.requiresTargetSelection || targets.requiresSelection;
        preview.outcome.usesRandomTarget = preview.outcome.usesRandomTarget || targets.random;

        const int targetCount = targetCountForSummary(effect.target, targets);
        if (effect.target == EffectTarget::SingleEnemy || effect.target == EffectTarget::AllEnemies ||
            effect.target == EffectTarget::RandomEnemy) {
            preview.outcome.affectedEnemyCount = std::max(preview.outcome.affectedEnemyCount, targetCount);
        } else if (effect.target == EffectTarget::Self || effect.target == EffectTarget::Ally ||
                   effect.target == EffectTarget::AllAllies || effect.target == EffectTarget::RandomAlly) {
            preview.outcome.affectedAllyCount = std::max(preview.outcome.affectedAllyCount, targetCount);
        }

        const bool affectsSourceStress =
            effect.target == EffectTarget::Self || effect.target == EffectTarget::AllAllies;
        if (affectsSourceStress && effect.type == EffectType::GainStress) {
            sourceStressDeltaMinimum += scaledMinimum * repetitions;
            sourceStressDeltaMaximum += scaledMaximum * repetitions;
        } else if (affectsSourceStress && effect.type == EffectType::LoseStress) {
            sourceStressDeltaMinimum -= scaledMaximum * repetitions;
            sourceStressDeltaMaximum -= scaledMinimum * repetitions;
        }

        CombatState postStressSpendState;
        const CombatState* modifierState = &state;
        if (isStressConversionEffect(effect.type)) {
            effectPreview.stressCost = resolved.minimum;
            effectPreview.value = PreviewValue{effect.outputAmount, effect.outputAmount};
            const int conversionCost = resolved.minimum * repetitions;
            cumulativeStressCost += conversionCost;
            sourceStressDeltaMinimum -= conversionCost;
            sourceStressDeltaMaximum -= conversionCost;
            if (state.hasEntity(source)) {
                postStressSpendState = state;
                CombatEntity& previewSource = postStressSpendState.entity(source);
                previewSource.stress = std::max(0, previewSource.stress - cumulativeStressCost);
                modifierState = &postStressSpendState;
            }
        }

        if (effect.type == EffectType::Damage || effect.type == EffectType::SpendStressDamage) {
            const int rawMinimum = effect.type == EffectType::SpendStressDamage ? effect.outputAmount : resolved.minimum;
            const int rawMaximum = effect.type == EffectType::SpendStressDamage ? effect.outputAmount : resolved.maximum;
            std::vector<PreviewValue> modifiedByTarget;
            std::vector<PreviewValue> hpByTarget;

            for (const EntityId effectTarget : targets.candidates) {
                const int targetScalingBonus = effect.type == EffectType::SpendStressDamage
                    ? 0
                    : effectScalingBonus(*modifierState, effect, source, effectTarget);
                const DamagePreview perHitDamage = damageSystem_.previewDamage(
                    *modifierState,
                    source,
                    effectTarget,
                    std::max(0, rawMinimum + targetScalingBonus),
                    std::max(0, rawMaximum + targetScalingBonus),
                    definition.id,
                    definition.diceCorruption,
                    true
                );
                PreviewBlockState targetBlockState = blockStateFor(previewBlockStates, state, effectTarget);
                DamagePreview totalDamage = applyRepeatedDamagePreview(
                    perHitDamage,
                    repetitions,
                    targetBlockState
                );
                if (!targets.random) {
                    blockStateFor(previewBlockStates, state, effectTarget) = targetBlockState;
                }
                if (!effectPreview.damage.has_value()) {
                    effectPreview.damage = totalDamage;
                }
                appendUnique(preview.outcome.modifierLabels, perHitDamage.modifierLabels);
                modifiedByTarget.push_back({totalDamage.modifiedMin, totalDamage.modifiedMax});
                hpByTarget.push_back({totalDamage.hpDamageMin, totalDamage.hpDamageMax});
            }

            if (targets.candidates.empty()) {
                const DamagePreview damage = damageSystem_.previewOutgoingDamage(
                    *modifierState,
                    source,
                    std::max(0, rawMinimum + representativeScalingBonus),
                    std::max(0, rawMaximum + representativeScalingBonus),
                    definition.id,
                    definition.diceCorruption,
                    true
                );
                DamagePreview totalDamage = damage;
                totalDamage.rawMin *= repetitions;
                totalDamage.rawMax *= repetitions;
                totalDamage.modifiedMin *= repetitions;
                totalDamage.modifiedMax *= repetitions;
                totalDamage.hpDamageMin *= repetitions;
                totalDamage.hpDamageMax *= repetitions;
                effectPreview.damage = totalDamage;
                appendUnique(preview.outcome.modifierLabels, damage.modifierLabels);
                modifiedByTarget.push_back({totalDamage.modifiedMin, totalDamage.modifiedMax});
                hpByTarget.push_back({totalDamage.hpDamageMin, totalDamage.hpDamageMax});
            }

            if (targets.random) {
                const PreviewValue modified = randomRange(modifiedByTarget);
                const PreviewValue hp = randomRange(hpByTarget);
                addRange(preview.outcome.modifiedDamage, modified.minimum, modified.maximum);
                addRange(preview.outcome.hpDamage, hp.minimum, hp.maximum);
            } else {
                for (const PreviewValue& value : modifiedByTarget) {
                    addRange(preview.outcome.modifiedDamage, value.minimum, value.maximum);
                }
                for (const PreviewValue& value : hpByTarget) {
                    addRange(preview.outcome.hpDamage, value.minimum, value.maximum);
                }
            }
        }

        if (effect.type == EffectType::Block || effect.type == EffectType::SpendStressBlock) {
            const int rawMinimum = effect.type == EffectType::SpendStressBlock ? effect.outputAmount : resolved.minimum;
            const int rawMaximum = effect.type == EffectType::SpendStressBlock ? effect.outputAmount : resolved.maximum;
            std::vector<PreviewValue> blockByTarget;
            const std::vector<EntityId> blockTargets = targets.candidates.empty()
                ? std::vector<EntityId>{source}
                : targets.candidates;

            for (const EntityId blockTarget : blockTargets) {
                const int targetScalingBonus = effect.type == EffectType::SpendStressBlock
                    ? 0
                    : effectScalingBonus(*modifierState, effect, source, blockTarget);
                const ModifiedValueRange blockRange = blockSystem_.previewBlock(
                    *modifierState,
                    source,
                    blockTarget,
                    std::max(0, rawMinimum + targetScalingBonus),
                    std::max(0, rawMaximum + targetScalingBonus),
                    definition.id,
                    definition.diceCorruption,
                    true
                );
                if (blockByTarget.empty()) {
                    effectPreview.value = PreviewValue{blockRange.modifiedMin, blockRange.modifiedMax};
                }
                appendUnique(preview.outcome.modifierLabels, blockRange.modifierDescriptions);
                const PreviewValue totalBlock{
                    blockRange.modifiedMin * repetitions,
                    blockRange.modifiedMax * repetitions
                };
                blockByTarget.push_back(totalBlock);
                if (!targets.random) {
                    PreviewBlockState& simulatedBlock = blockStateFor(previewBlockStates, state, blockTarget);
                    simulatedBlock.minimum += totalBlock.minimum;
                    simulatedBlock.maximum += totalBlock.maximum;
                }
            }

            if (targets.random) {
                const PreviewValue block = randomRange(blockByTarget);
                addRange(preview.outcome.block, block.minimum, block.maximum);
            } else {
                for (const PreviewValue& block : blockByTarget) {
                    addRange(preview.outcome.block, block.minimum, block.maximum);
                }
            }
        }

        if (effect.type == EffectType::Heal) {
            const int count = std::max(1, targetCount);
            const int multiplier = targets.random ? 1 : count;
            addRange(
                preview.outcome.healing,
                scaledMinimum * repetitions * multiplier,
                scaledMaximum * repetitions * multiplier
            );
        }

        preview.effects.push_back(std::move(effectPreview));
    }

    preview.sourceStressAfterMinimum = std::clamp(
        preview.sourceStressBefore + sourceStressDeltaMinimum,
        0,
        preview.sourceMaxStress
    );
    preview.sourceStressAfterMaximum = std::clamp(
        preview.sourceStressBefore + sourceStressDeltaMaximum,
        0,
        preview.sourceMaxStress
    );
    preview.wouldCollapseFromStress =
        preview.sourceMaxStress > 0 &&
        preview.sourceStressBefore + sourceStressDeltaMaximum >= preview.sourceMaxStress;

    return preview;
}
