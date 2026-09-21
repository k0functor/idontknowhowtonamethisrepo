#include "ContentValidator.hpp"

#include "active_items/ActiveItemId.hpp"
#include "cards/CardId.hpp"
#include "cards/CardRarity.hpp"
#include "cards/CardUpgrade.hpp"
#include "consumables/ConsumableId.hpp"
#include "drones/DroneId.hpp"
#include "effects/EffectDefinition.hpp"
#include "enemies/EnemyId.hpp"
#include "localization/Locale.hpp"
#include "localization/LocalizationManager.hpp"
#include "localization/TextId.hpp"
#include "relics/RelicId.hpp"
#include "rewards/RewardPoolRules.hpp"
#include "run/DifficultyId.hpp"
#include "run/RunMapNode.hpp"
#include "statuses/StatusId.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

namespace {
void addError(std::vector<std::string>& errors, std::string message) {
    errors.push_back(std::move(message));
}

int rarityRank(const CardRarity rarity) {
    switch (rarity) {
        case CardRarity::Starter:
            return 0;
        case CardRarity::Common:
            return 1;
        case CardRarity::Uncommon:
            return 2;
        case CardRarity::Rare:
            return 3;
        case CardRarity::Special:
            return 4;
    }

    return 0;
}

bool meetsMinimumCardRarity(const CardDefinition& card, const std::optional<CardRarity>& minimumRarity) {
    if (!minimumRarity.has_value()) {
        return true;
    }

    return rarityRank(card.rarity) >= rarityRank(*minimumRarity);
}

std::string rewardPoolKey(const CardDefinition& card) {
    if (!card.rewardPoolId.empty()) {
        return card.rewardPoolId;
    }

    return card.ownerActorId;
}

void validateTextReference(
    std::vector<std::string>& errors,
    const LocalizationManager* localization,
    const std::string& owner,
    const std::string& fieldName,
    const TextId& textId
) {
    if (localization == nullptr) {
        return;
    }

    const std::array<Locale, 2> requiredLocales = {
        Locale::russian(),
        Locale::english()
    };

    for (const Locale& locale : requiredLocales) {
        if (!localization->hasText(locale, textId)) {
            addError(
                errors,
                owner + " references missing localization text '" + textId.value +
                    "' in field '" + fieldName + "' for locale '" + locale.code() + "'"
            );
        }
    }
}

void validateOptionalTextReference(
    std::vector<std::string>& errors,
    const LocalizationManager* localization,
    const std::string& owner,
    const std::string& fieldName,
    const std::optional<TextId>& textId
) {
    if (!textId.has_value()) {
        return;
    }

    validateTextReference(errors, localization, owner, fieldName, *textId);
}

void validateTextReferenceList(
    std::vector<std::string>& errors,
    const LocalizationManager* localization,
    const std::string& owner,
    const std::string& fieldName,
    const std::vector<TextId>& textIds
) {
    for (std::size_t index = 0; index < textIds.size(); ++index) {
        validateTextReference(
            errors,
            localization,
            owner,
            fieldName + "[" + std::to_string(index) + "]",
            textIds[index]
        );
    }
}


void validateUnlockReward(
    std::vector<std::string>& errors,
    const ContentRegistry& content,
    const std::string& owner,
    const UnlockReward& reward
) {
    for (const std::string& archetypeId : reward.archetypeIds) {
        if (!content.archetypes().contains(PlayableArchetypeId(archetypeId))) {
            addError(errors, owner + " reward unlocks unknown archetype '" + archetypeId + "'");
        }
    }

    for (const std::string& cardId : reward.cardIds) {
        if (!content.cards().contains(CardId(cardId))) {
            addError(errors, owner + " reward unlocks unknown card '" + cardId + "'");
        }
    }

    for (const std::string& relicId : reward.relicIds) {
        if (!content.relics().contains(RelicId(relicId))) {
            addError(errors, owner + " reward unlocks unknown relic '" + relicId + "'");
        }
    }
}

void validateStatusReference(
    std::vector<std::string>& errors,
    const ContentRegistry& content,
    const std::string& owner,
    const EffectDefinition& effect
) {
    if (!effect.statusId.has_value()) {
        return;
    }

    if (effect.type == EffectType::SummonDrone) {
        if (!content.drones().contains(DroneId(*effect.statusId))) {
            addError(errors, owner + " references unknown drone '" + *effect.statusId + "'");
        }
        return;
    }

    if (effect.type == EffectType::PrimeStressBreakdown) {
        const std::string& type = *effect.statusId;
        if (type != "discard" && type != "energy" && type != "status_cards" && type != "cost" && type != "frenzy") {
            addError(errors, owner + " references invalid stress breakdown type '" + type + "'");
        }
        return;
    }

    const StatusId statusId(*effect.statusId);
    if (!content.statuses().contains(statusId)) {
        addError(errors, owner + " references unknown status '" + *effect.statusId + "'");
    }
}

void validateEnemyAiStatusList(
    std::vector<std::string>& errors,
    const ContentRegistry& content,
    const std::string& owner,
    const std::string& fieldName,
    const std::vector<std::string>& statusIds
) {
    for (const std::string& statusId : statusIds) {
        if (!content.statuses().contains(StatusId(statusId))) {
            addError(
                errors,
                owner + " references unknown status '" + statusId +
                    "' in AI condition '" + fieldName + "'"
            );
        }
    }
}

void validateEnemyActionAi(
    std::vector<std::string>& errors,
    const ContentRegistry& content,
    const std::string& owner,
    const EnemyActionDefinition& action
) {
    const EnemyActionCondition& condition = action.condition;
    validateEnemyAiStatusList(
        errors, content, owner, "required_self_statuses", condition.requiredSelfStatuses
    );
    validateEnemyAiStatusList(
        errors, content, owner, "forbidden_self_statuses", condition.forbiddenSelfStatuses
    );
    validateEnemyAiStatusList(
        errors, content, owner, "required_player_statuses", condition.requiredPlayerStatuses
    );
    validateEnemyAiStatusList(
        errors, content, owner, "forbidden_player_statuses", condition.forbiddenPlayerStatuses
    );

    for (const std::string& statusId : condition.requiredSelfStatuses) {
        if (std::find(
                condition.forbiddenSelfStatuses.begin(),
                condition.forbiddenSelfStatuses.end(),
                statusId
            ) != condition.forbiddenSelfStatuses.end()) {
            addError(errors, owner + " both requires and forbids self status '" + statusId + "'");
        }
    }
    for (const std::string& statusId : condition.requiredPlayerStatuses) {
        if (std::find(
                condition.forbiddenPlayerStatuses.begin(),
                condition.forbiddenPlayerStatuses.end(),
                statusId
            ) != condition.forbiddenPlayerStatuses.end()) {
            addError(errors, owner + " both requires and forbids player status '" + statusId + "'");
        }
    }
}

void validateEffectList(
    std::vector<std::string>& errors,
    const ContentRegistry& content,
    const std::string& owner,
    const std::vector<EffectDefinition>& effects
) {
    for (const EffectDefinition& effect : effects) {
        validateStatusReference(errors, content, owner, effect);

        if (effect.scaling.statusId.has_value() &&
            !content.statuses().contains(StatusId(*effect.scaling.statusId))) {
            addError(errors, owner + " scaling references unknown status '" + *effect.scaling.statusId + "'");
        }

        if (effect.type == EffectType::RecoverCards && effect.target != EffectTarget::Self) {
            addError(errors, owner + " has recover_cards with non-self target '" + toString(effect.target) + "'");
        }

        if (isStressConversionEffect(effect.type)) {
            if (!effect.value.isFixed() ||
                effect.value.minimumPossibleValue() <= 0 ||
                effect.outputAmount <= 0) {
                addError(errors, owner + " has an invalid stress conversion effect");
            }

            const bool damageTargetValid = effect.type != EffectType::SpendStressDamage ||
                effect.target == EffectTarget::SingleEnemy ||
                effect.target == EffectTarget::AllEnemies;
            const bool selfTargetValid = effect.type == EffectType::SpendStressDamage ||
                effect.target == EffectTarget::Self;
            if (!damageTargetValid || !selfTargetValid) {
                addError(errors, owner + " has stress conversion effect '" + toString(effect.type) +
                    "' with invalid target '" + toString(effect.target) + "'");
            }

            if (!effect.scaling.empty()) {
                addError(errors, owner + " has stress conversion effect with unsupported scaling");
            }
        }

        if (effect.type == EffectType::PrimeStressBreakdown) {
            if (!effect.statusId.has_value()) {
                addError(errors, owner + " has prime_stress_breakdown without breakdown type");
            }
            if (effect.target != EffectTarget::RandomAlly && effect.target != EffectTarget::AllAllies && effect.target != EffectTarget::Self) {
                addError(errors, owner + " has prime_stress_breakdown with invalid target '" + toString(effect.target) + "'");
            }
        }

        if (effect.type == EffectType::UseDrone) {
            if (effect.target != EffectTarget::Self) {
                addError(errors, owner + " has use_drone effect with target '" + toString(effect.target) + "'; use_drone must target self");
            }

            if (effect.statusId.has_value()) {
                addError(errors, owner + " has use_drone effect with status/drone id '" + *effect.statusId + "'; use_drone activates the oldest available drone and must not name a drone directly");
            }
        }
    }
}

bool hasCardRewardCandidates(const ContentRegistry& content) {
    for (const CardDefinition* card : content.cards().all()) {
        if (card != nullptr && RewardPoolRules::canAppearAsCardReward(*card)) {
            return true;
        }
    }

    return false;
}

bool hasCardRewardCandidatesAtMinimum(
    const ContentRegistry& content,
    const std::optional<CardRarity>& minimumRarity
) {
    for (const CardDefinition* card : content.cards().all()) {
        if (card != nullptr &&
            RewardPoolRules::canAppearAsCardReward(*card) &&
            meetsMinimumCardRarity(*card, minimumRarity)) {
            return true;
        }
    }

    return false;
}

bool hasArchetypeCardRewardCandidates(
    const ContentRegistry& content,
    const std::vector<std::string>& rewardCardPoolIds
) {
    for (const CardDefinition* card : content.cards().all()) {
        if (card == nullptr) {
            continue;
        }

        if (!RewardPoolRules::canAppearAsCardReward(*card)) {
            continue;
        }

        const std::string pool = rewardPoolKey(*card);
        if (pool.empty()) {
            continue;
        }

        if (std::find(rewardCardPoolIds.begin(), rewardCardPoolIds.end(), pool) != rewardCardPoolIds.end()) {
            return true;
        }
    }

    return false;
}

bool hasRelicRewardCandidates(const ContentRegistry& content) {
    for (const RelicDefinition* relic : content.relics().all()) {
        if (relic != nullptr && RewardPoolRules::canAppearAsRelicReward(*relic)) {
            return true;
        }
    }

    return false;
}

bool hasShopCardCandidates(const ContentRegistry& content) {
    for (const CardDefinition* card : content.cards().all()) {
        if (card != nullptr && RewardPoolRules::canAppearInShop(*card)) {
            return true;
        }
    }

    return false;
}

bool hasShopRelicCandidates(const ContentRegistry& content) {
    for (const RelicDefinition* relic : content.relics().all()) {
        if (relic != nullptr && RewardPoolRules::canAppearInShop(*relic)) {
            return true;
        }
    }

    return false;
}

bool hasConsumableCandidates(const ContentRegistry& content) {
    return content.consumables().size() > 0;
}

void validateRunEventEffect(
    std::vector<std::string>& errors,
    const ContentRegistry& content,
    const std::string& owner,
    const RunEventEffect& effect
) {
    switch (effect.type) {
        case RunEventEffectType::GainGold:
        case RunEventEffectType::LoseGold:
        case RunEventEffectType::GainStress:
        case RunEventEffectType::LoseStress:
        case RunEventEffectType::LoseHp:
        case RunEventEffectType::HealAll:
            if (effect.amount <= 0) {
                addError(errors, owner + " has numeric event effect with non-positive amount");
            }
            break;
        case RunEventEffectType::GainCard:
        case RunEventEffectType::RemoveCard:
            if (effect.contentId.empty()) {
                addError(errors, owner + " has card event effect without content id");
            } else if (!content.cards().contains(CardId(effect.contentId))) {
                addError(errors, owner + " references unknown card '" + effect.contentId + "'");
            }
            break;
        case RunEventEffectType::GainRandomCard:
            if (!hasCardRewardCandidates(content)) {
                addError(errors, owner + " can generate a random card, but the card reward pool is empty");
            }
            break;
        case RunEventEffectType::GainRelic:
            if (effect.contentId.empty()) {
                addError(errors, owner + " has relic event effect without content id");
            } else if (!content.relics().contains(RelicId(effect.contentId))) {
                addError(errors, owner + " references unknown relic '" + effect.contentId + "'");
            }
            break;
        case RunEventEffectType::GainRandomRelic:
            if (!hasRelicRewardCandidates(content)) {
                addError(errors, owner + " can generate a random relic, but the relic reward pool is empty");
            }
            break;
        case RunEventEffectType::GainConsumable:
            if (effect.contentId.empty()) {
                addError(errors, owner + " has consumable event effect without content id");
            } else if (!content.consumables().contains(ConsumableId(effect.contentId))) {
                addError(errors, owner + " references unknown consumable '" + effect.contentId + "'");
            }
            break;
        case RunEventEffectType::GainRandomConsumable:
            if (!hasConsumableCandidates(content)) {
                addError(errors, owner + " can generate a random consumable, but the consumable pool is empty");
            }
            break;
        case RunEventEffectType::RemoveRandomCard:
            if (effect.amount != 0) {
                addError(errors, owner + " has remove_random_card event effect with a non-zero amount");
            }
            break;
        case RunEventEffectType::UpgradeRandomCard:
            if (effect.amount != 0) {
                addError(errors, owner + " has upgrade_random_card event effect with a non-zero amount");
            }
            break;
        case RunEventEffectType::SetFlag:
        case RunEventEffectType::ClearFlag:
            if (effect.contentId.empty()) {
                addError(errors, owner + " has event flag effect without flag id");
            }
            if (effect.amount != 0) {
                addError(errors, owner + " has event flag effect with a non-zero amount");
            }
            break;
        case RunEventEffectType::Skip:
            if (effect.amount != 0) {
                addError(errors, owner + " has skip event effect with a non-zero amount");
            }
            break;
    }
}


int configuredLayerWidth(const RunMapGenerationConfig& config, const int layer) {
    if (layer < 0 || layer >= config.layerCount()) {
        return 0;
    }

    if (config.hasLayerNodeCounts()) {
        return config.layerNodeCounts()[static_cast<std::size_t>(layer)];
    }

    return (layer == 0 || layer == config.layerCount() - 2 || layer == config.layerCount() - 1)
        ? 1
        : config.middleMaxNodes();
}

bool hasEncounterForType(const EncounterDatabase& encounters, const RunMapNodeType nodeType) {
    for (const EncounterDefinition* encounter : encounters.all()) {
        if (encounter != nullptr && encounter->nodeType == nodeType) {
            return true;
        }
    }

    return false;
}

bool hasEncounterEligibleOnLayer(
    const EncounterDatabase& encounters,
    const RunMapNodeType nodeType,
    const int layer
) {
    for (const EncounterDefinition* encounter : encounters.all()) {
        if (encounter != nullptr &&
            encounter->nodeType == nodeType &&
            encounter->isAllowedOnLayer(layer)) {
            return true;
        }
    }

    return false;
}

bool hasEncounterEligibleInLayerRange(
    const EncounterDatabase& encounters,
    const RunMapNodeType nodeType,
    const int minLayer,
    const int maxLayer
) {
    for (int layer = minLayer; layer <= maxLayer; ++layer) {
        if (hasEncounterEligibleOnLayer(encounters, nodeType, layer)) {
            return true;
        }
    }

    return false;
}

bool hasAvailableArchetype(const ContentRegistry& content) {
    for (const PlayableArchetypeDefinition* archetype : content.archetypes().all()) {
        if (archetype != nullptr && archetype->isAvailable) {
            return true;
        }
    }

    return false;
}

bool hasUpgradableCardForAvailableArchetype(const ContentRegistry& content) {
    std::set<std::string> relevantActorIds;

    for (const PlayableArchetypeDefinition* archetype : content.archetypes().all()) {
        if (archetype == nullptr || !archetype->isAvailable) {
            continue;
        }

        relevantActorIds.insert(archetype->rewardCardPoolIds.begin(), archetype->rewardCardPoolIds.end());

        for (const std::string& cardId : archetype->startingDeckCardIds) {
            const CardId id(cardId);
            if (!content.cards().contains(id)) {
                continue;
            }

            if (CardUpgrade::isUpgradable(content.cards().get(id))) {
                return true;
            }
        }
    }

    for (const CardDefinition* card : content.cards().all()) {
        if (card == nullptr) {
            continue;
        }

        const std::string pool = rewardPoolKey(*card);
        if (pool.empty()) {
            continue;
        }

        if (relevantActorIds.contains(pool) &&
            RewardPoolRules::canAppearAsCardReward(*card) &&
            CardUpgrade::isUpgradable(*card)) {
            return true;
        }
    }

    return false;
}

void validateCardUpgradeDefinition(
    std::vector<std::string>& errors,
    const ContentRegistry& content,
    const std::string& owner,
    const CardDefinition& card
) {
    if (card.upgrade.empty()) {
        return;
    }

    if (card.upgrade.effects.has_value() && card.upgrade.effects->empty()) {
        addError(errors, owner + " upgrade overrides effects with an empty list");
    }

    const CardDefinition upgraded = CardUpgrade::upgradedDefinition(card);
    if (upgraded.energyCost < 0) {
        addError(errors, owner + " upgrade produces a negative energy cost");
    }
    if (upgraded.goldCost < 0) {
        addError(errors, owner + " upgrade produces a negative gold cost");
    }
    if (upgraded.effects.empty()) {
        addError(errors, owner + " upgrade produces a card with no effects");
    }

    validateEffectList(errors, content, owner + " upgraded definition", upgraded.effects);
}

void validateFloorContent(
    std::vector<std::string>& errors,
    const ContentRegistry& content,
    const FloorDefinition& floor,
    const RunMapGenerationConfig& config,
    const EncounterDatabase& encounters
) {
    const int preBossLayer = config.layerCount() - 2;
    const int bossLayer = config.layerCount() - 1;

    if (configuredLayerWidth(config, 0) != 1) {
        addError(errors, "Act '" + config.id() + "' must start with exactly one combat room");
    }

    if (configuredLayerWidth(config, preBossLayer) <= 0) {
        addError(errors, "Act '" + config.id() + "' must have at least one rest room before the boss");
    }

    if (configuredLayerWidth(config, bossLayer) != 1) {
        addError(errors, "Act '" + config.id() + "' must end with exactly one boss room");
    }

    if (!hasEncounterForType(encounters, RunMapNodeType::Combat)) {
        addError(errors, "Floor '" + floor.id + "' / map '" + config.id() + "' needs at least one normal combat encounter");
    } else if (!hasEncounterEligibleOnLayer(encounters, RunMapNodeType::Combat, 0)) {
        addError(errors, "Act '" + config.id() + "' has no normal combat encounter eligible for the start layer");
    }

    if (config.combatWeight() > 0) {
        const int lastRandomCombatLayer = std::max(0, preBossLayer - 1);
        if (!hasEncounterEligibleInLayerRange(encounters, RunMapNodeType::Combat, 1, lastRandomCombatLayer)) {
            addError(errors, "Act '" + config.id() + "' can generate normal combat rooms, but no normal encounter is eligible for middle layers");
        }
    }

    const bool canResolveEventsToCombat = config.questionMarkCombatChance() > 0 &&
        (config.eventWeight() > 0 || (config.hasFixedEvents() && config.events().maximum > 0));
    if (canResolveEventsToCombat && !hasEncounterEligibleInLayerRange(encounters, RunMapNodeType::Combat, 1, preBossLayer - 1)) {
        addError(errors, "Act '" + config.id() + "' event rooms can turn into combat, but no normal encounter is eligible for event layers");
    }

    if (config.elites().maximum > 0) {
        if (!hasEncounterForType(encounters, RunMapNodeType::Elite)) {
            addError(errors, "Act '" + config.id() + "' can generate elite rooms, but the elite encounter pool is empty");
        } else if (!hasEncounterEligibleInLayerRange(encounters, RunMapNodeType::Elite, config.elites().minLayer, config.elites().maxLayer)) {
            addError(errors, "Act '" + config.id() + "' can generate elite rooms, but no elite encounter is eligible for configured elite layers");
        }

        if (!content.rewardTuning().node(RunMapNodeType::Elite).guaranteedRelic) {
            addError(errors, "Act '" + config.id() + "' has elite rooms, but elite reward tuning does not guarantee a relic");
        }
    }

    if (!hasEncounterForType(encounters, RunMapNodeType::Boss)) {
        addError(errors, "Act '" + config.id() + "' needs at least one boss encounter");
    } else if (!hasEncounterEligibleOnLayer(encounters, RunMapNodeType::Boss, bossLayer)) {
        addError(errors, "Act '" + config.id() + "' has no boss encounter eligible for the boss layer");
    }

    const NodeRewardTuning& bossReward = content.rewardTuning().node(RunMapNodeType::Boss);
    if (bossReward.gold <= 0 && !bossReward.offerCards && !bossReward.guaranteedRelic) {
        addError(errors, "Act '" + config.id() + "' boss reward tuning gives no gold, cards, or relics");
    }
    if (!bossReward.guaranteedRelic) {
        addError(errors, "Act '" + config.id() + "' has a boss room, but boss reward tuning does not guarantee a relic");
    }

    const bool canGenerateEventNodes = config.eventWeight() > 0 ||
        (config.hasFixedEvents() && config.events().maximum > 0);
    if (canGenerateEventNodes && content.events().allForPool(floor.eventPoolId).empty()) {
        addError(errors, "Floor '" + floor.id + "' / map '" + config.id() + "' can generate event rooms, but event pool '" + floor.eventPoolId + "' is empty");
    }

    if (config.chests().count > 0 && !hasRelicRewardCandidates(content)) {
        addError(errors, "Act '" + config.id() + "' has chest rooms, but the relic reward pool is empty");
    }

    if (config.shop().count > 0 &&
        content.shopTuning().cardOfferCount() <= 0 &&
        content.shopTuning().relicOfferCount() <= 0 &&
        content.shopTuning().consumableOfferCount() <= 0 &&
        content.shopTuning().cardRemovalPrice() <= 0) {
        addError(errors, "Act '" + config.id() + "' has shop rooms, but shop tuning exposes no offers or services");
    }

    if (!hasAvailableArchetype(content)) {
        addError(errors, "Act '" + config.id() + "' cannot start because there are no available playable archetypes");
    }

    if (content.difficulties().size() == 0) {
        addError(errors, "Act '" + config.id() + "' cannot start because there are no difficulties loaded");
    }

    if (!hasUpgradableCardForAvailableArchetype(content)) {
        addError(errors, "Act '" + config.id() + "' rest room has upgrade action, but no available archetype has an upgradable starting or reward card");
    }
}

void validateRewardTuning(
    std::vector<std::string>& errors,
    const ContentRegistry& content
) {
    const std::array<RunMapNodeType, 7> nodeTypes = {
        RunMapNodeType::Combat,
        RunMapNodeType::Elite,
        RunMapNodeType::Event,
        RunMapNodeType::Shop,
        RunMapNodeType::Chest,
        RunMapNodeType::Rest,
        RunMapNodeType::Boss
    };

    const bool cardRewardPoolAvailable = hasCardRewardCandidates(content);
    const bool relicRewardPoolAvailable = hasRelicRewardCandidates(content);
    const bool consumableRewardPoolAvailable = hasConsumableCandidates(content);

    for (const RunMapNodeType nodeType : nodeTypes) {
        const NodeRewardTuning& tuning = content.rewardTuning().node(nodeType);
        const std::string owner = "Reward tuning for node type '" + toString(nodeType) + "'";

        if (tuning.offerCards && tuning.cardChoices <= 0) {
            addError(errors, owner + " offers cards but card_choices is not positive");
        }

        if (!tuning.offerCards && tuning.cardChoices > 0) {
            addError(errors, owner + " disables card offers but still sets card_choices");
        }

        if (tuning.offerCards && tuning.cardChoices > 0 && !cardRewardPoolAvailable) {
            addError(errors, owner + " offers cards, but the card reward pool is empty");
        }

        if (tuning.offerCards && tuning.cardChoices > 0 &&
            tuning.minimumCardRarity.has_value() &&
            !hasCardRewardCandidatesAtMinimum(content, tuning.minimumCardRarity)) {
            addError(errors, owner + " requires a minimum card rarity, but no matching card reward candidates exist");
        }

        if (tuning.guaranteedRelic && !relicRewardPoolAvailable) {
            addError(errors, owner + " guarantees a relic, but the relic reward pool is empty");
        }

        if (tuning.consumableChancePercent < 0 || tuning.consumableChancePercent > 100) {
            addError(errors, owner + " has consumable_chance_percent outside 0..100");
        }

        if (tuning.consumableChancePercent > 0 && !consumableRewardPoolAvailable) {
            addError(errors, owner + " can offer a consumable, but the consumable pool is empty");
        }
    }
}

void validateShopTuning(
    std::vector<std::string>& errors,
    const ContentRegistry& content
) {
    if (content.shopTuning().cardOfferCount() > 0 && !hasShopCardCandidates(content)) {
        addError(errors, "Shop tuning offers cards, but the shop card pool is empty");
    }

    if (content.shopTuning().relicOfferCount() > 0 && !hasShopRelicCandidates(content)) {
        addError(errors, "Shop tuning offers relics, but the shop relic pool is empty");
    }

    if (content.shopTuning().consumableOfferCount() > 0 && !hasConsumableCandidates(content)) {
        addError(errors, "Shop tuning offers consumables, but the consumable pool is empty");
    }
}

void validateMapGeneration(
    std::vector<std::string>& errors,
    const ContentRegistry& content,
    const LocalizationManager* localization
) {
    for (const ChallengeDefinition* challenge : content.challenges().all()) {
        if (challenge == nullptr) {
            continue;
        }

        const std::string owner = "Challenge '" + challenge->id + "'";
        validateTextReference(errors, localization, owner, "name", challenge->nameTextId);
        validateTextReference(errors, localization, owner, "description", challenge->descriptionTextId);
        validateTextReference(errors, localization, owner, "goal", challenge->goalTextId);
        validateTextReference(errors, localization, owner, "reward", challenge->rewardTextId);
        validateTextReference(errors, localization, owner, "unlock_hint", challenge->unlockHintTextId);
        validateUnlockReward(errors, content, owner, challenge->reward);

        for (const std::string& archetypeId : challenge->requiredUnlockedArchetypeIds) {
            if (!content.archetypes().contains(PlayableArchetypeId(archetypeId))) {
                addError(errors, owner + " requires unknown unlocked archetype '" + archetypeId + "'");
            }
        }

        for (const std::string& requiredChallengeId : challenge->requiredCompletedChallengeIds) {
            if (!content.challenges().contains(requiredChallengeId)) {
                addError(errors, owner + " requires unknown completed challenge '" + requiredChallengeId + "'");
            }
        }

        if (challenge->startingArchetypeId.empty()) {
            addError(errors, owner + " has empty starting archetype id");
        } else if (!content.archetypes().contains(PlayableArchetypeId(challenge->startingArchetypeId))) {
            addError(errors, owner + " references unknown starting archetype '" + challenge->startingArchetypeId + "'");
        }

        if (challenge->startingDifficultyId.empty()) {
            addError(errors, owner + " has empty starting difficulty id");
        } else if (!content.difficulties().contains(DifficultyId(challenge->startingDifficultyId))) {
            addError(errors, owner + " references unknown starting difficulty '" + challenge->startingDifficultyId + "'");
        }

        if (!challenge->startingFloorId.empty() && !content.floors().contains(challenge->startingFloorId)) {
            addError(errors, owner + " references unknown starting floor '" + challenge->startingFloorId + "'");
        }

        if (challenge->startingGoldOverride < -1) {
            addError(errors, owner + " has starting gold below -1");
        }

        for (const std::string& cardId : challenge->fixedStartingDeckCardIds) {
            if (!content.cards().contains(CardId(cardId))) {
                addError(errors, owner + " fixed starting deck references unknown card '" + cardId + "'");
            }
        }

        for (const std::string& relicId : challenge->fixedStartingRelicIds) {
            if (!content.relics().contains(RelicId(relicId))) {
                addError(errors, owner + " fixed starting relics reference unknown relic '" + relicId + "'");
            }
        }

        for (const std::string& consumableId : challenge->fixedStartingConsumableIds) {
            if (!content.consumables().contains(ConsumableId(consumableId))) {
                addError(errors, owner + " fixed starting consumables reference unknown consumable '" + consumableId + "'");
            }
        }

        const ChallengeCompletionCondition& completion = challenge->completion;
        if (completion.type != "clear_floor" &&
            completion.type != "clear_floor_without_shop" &&
            completion.type != "clear_floor_with_elites" &&
            completion.type != "clear_floor_with_archetype" &&
            completion.type != "clear_floor_low_damage") {
            addError(errors, owner + " has unknown completion type '" + completion.type + "'");
        }

        if (!completion.floorId.empty() && !content.floors().contains(completion.floorId)) {
            addError(errors, owner + " references unknown completion floor '" + completion.floorId + "'");
        }

        if (!completion.archetypeId.empty() && !content.archetypes().contains(PlayableArchetypeId(completion.archetypeId))) {
            addError(errors, owner + " references unknown completion archetype '" + completion.archetypeId + "'");
        }

        if (completion.minElitesKilled < 0 || completion.minBossesKilled < 0) {
            addError(errors, owner + " has negative minimum kill counters");
        }

        if (completion.maxShopsVisited < -1 || completion.maxDamageTaken < -1) {
            addError(errors, owner + " has invalid max counter below -1");
        }
    }



    for (const AchievementDefinition* achievement : content.achievements().all()) {
        if (achievement == nullptr) {
            continue;
        }

        const std::string owner = "Achievement '" + achievement->id + "'";
        validateTextReference(errors, localization, owner, "name", achievement->nameTextId);
        validateTextReference(errors, localization, owner, "description", achievement->descriptionTextId);
        validateTextReference(errors, localization, owner, "goal", achievement->goalTextId);
        validateTextReference(errors, localization, owner, "reward", achievement->rewardTextId);
        validateTextReference(errors, localization, owner, "unlock_hint", achievement->unlockHintTextId);
        validateUnlockReward(errors, content, owner, achievement->reward);

        for (const std::string& archetypeId : achievement->requiredUnlockedArchetypeIds) {
            if (!content.archetypes().contains(PlayableArchetypeId(archetypeId))) {
                addError(errors, owner + " requires unknown unlocked archetype '" + archetypeId + "'");
            }
        }

        for (const std::string& requiredChallengeId : achievement->requiredCompletedChallengeIds) {
            if (!content.challenges().contains(requiredChallengeId)) {
                addError(errors, owner + " requires unknown completed challenge '" + requiredChallengeId + "'");
            }
        }

        for (const std::string& requiredAchievementId : achievement->requiredCompletedAchievementIds) {
            if (!content.achievements().contains(requiredAchievementId)) {
                addError(errors, owner + " requires unknown completed achievement '" + requiredAchievementId + "'");
            }
            if (requiredAchievementId == achievement->id) {
                addError(errors, owner + " cannot require itself");
            }
        }

        const AchievementCompletionCondition& completion = achievement->completion;
        if (completion.type != "clear_floor" &&
            completion.type != "clear_floor_with_elites" &&
            completion.type != "clear_floor_with_archetype" &&
            completion.type != "clear_floor_low_damage" &&
            completion.type != "complete_challenges" &&
            completion.type != "complete_achievements" &&
            completion.type != "win_runs" &&
            completion.type != "lose_runs") {
            addError(errors, owner + " has unknown completion type '" + completion.type + "'");
        }

        if (!completion.floorId.empty() && !content.floors().contains(completion.floorId)) {
            addError(errors, owner + " references unknown completion floor '" + completion.floorId + "'");
        }

        if (!completion.archetypeId.empty() && !content.archetypes().contains(PlayableArchetypeId(completion.archetypeId))) {
            addError(errors, owner + " references unknown completion archetype '" + completion.archetypeId + "'");
        }

        if (completion.minVictories < 0 ||
            completion.minDefeats < 0 ||
            completion.minCompletedChallenges < 0 ||
            completion.minCompletedAchievements < 0 ||
            completion.minElitesKilled < 0 ||
            completion.minBossesKilled < 0 ||
            completion.minEventsCompleted < 0 ||
            completion.minRelics < 0 ||
            completion.minGold < 0) {
            addError(errors, owner + " has negative minimum counters");
        }

        if (completion.maxDamageTaken < -1) {
            addError(errors, owner + " has invalid max damage below -1");
        }
    }

    for (const FloorDefinition* floor : content.floors().all()) {
        if (floor == nullptr || !floor->isImplemented) {
            continue;
        }

        const RunMapGenerationConfig& config = content.mapGenerationForFloor(floor->id);
        const bool canGenerateEventNodes =
            config.eventWeight() > 0 ||
            (config.hasFixedEvents() && config.events().maximum > 0);
        const bool canResolveQuestionMarksToEvents = config.questionMarkCombatChance() < 100;

        if (canGenerateEventNodes && canResolveQuestionMarksToEvents && content.events().allForPool(floor->eventPoolId).empty()) {
            addError(
                errors,
                "Floor '" + floor->id + "' / map '" + config.id() + "' can generate event nodes, but event pool '" + floor->eventPoolId + "' is empty"
            );
        }
    }
}

std::string joinErrors(const std::vector<std::string>& errors) {
    std::ostringstream out;
    out << "Content validation failed with " << errors.size() << " error(s):";

    for (const std::string& error : errors) {
        out << "\n - " << error;
    }

    return out.str();
}

void validateContent(const ContentRegistry& content, const LocalizationManager* localization) {
    std::vector<std::string> errors;

    for (const CardDefinition* card : content.cards().all()) {
        if (card == nullptr) {
            continue;
        }

        const std::string owner = "Card '" + card->id.value + "'";
        validateTextReference(errors, localization, owner, "name", card->nameTextId);
        validateTextReference(errors, localization, owner, "description", card->descriptionTextId);
        validateOptionalTextReference(errors, localization, owner, "upgrade.name", card->upgrade.nameTextId);
        validateOptionalTextReference(errors, localization, owner, "upgrade.description", card->upgrade.descriptionTextId);
        validateEffectList(errors, content, owner, card->effects);
        if (card->upgrade.effects.has_value()) {
            validateEffectList(errors, content, owner + " upgrade", *card->upgrade.effects);
        }
        validateCardUpgradeDefinition(errors, content, owner, *card);

        if (!card->ownerActorId.empty() && !content.actors().contains(PlayerActorId(card->ownerActorId))) {
            addError(errors, owner + " references unknown owner actor '" + card->ownerActorId + "'");
        }
    }

    for (const ConsumableDefinition* consumable : content.consumables().all()) {
        if (consumable == nullptr) {
            continue;
        }

        const std::string owner = "Consumable '" + consumable->id.value + "'";
        validateTextReference(errors, localization, owner, "name", consumable->nameTextId);
        validateTextReference(errors, localization, owner, "description", consumable->descriptionTextId);
        validateEffectList(errors, content, owner, consumable->effects);
    }

    for (const ActiveItemDefinition* item : content.activeItems().all()) {
        if (item == nullptr) {
            continue;
        }

        const std::string owner = "Active item '" + item->id.value + "'";
        validateTextReference(errors, localization, owner, "name", item->nameTextId);
        validateTextReference(errors, localization, owner, "description", item->descriptionTextId);
        if (item->maxCharge <= 0) {
            addError(errors, owner + " has non-positive max charge");
        }
        if (item->chargeCost <= 0 || item->chargeCost > item->maxCharge) {
            addError(errors, owner + " has charge cost outside 1..max_charge");
        }
        if (item->shopPrice < 0) {
            addError(errors, owner + " has negative shop price");
        }
        if (item->canAppearInShop && item->shopPrice <= 0) {
            addError(errors, owner + " is shop eligible but has no positive shop price");
        }
        if (item->useContexts.empty()) {
            addError(errors, owner + " has no use contexts");
        }
        if (item->effects.empty()) {
            addError(errors, owner + " has no effects");
        }
        for (const ActiveItemEffectDefinition& effect : item->effects) {
            if (effect.amount <= 0) {
                addError(errors, owner + " has an effect with non-positive amount");
            }
            const auto hasContext = [&](const ActiveItemUseContext context) {
                return std::find(item->useContexts.begin(), item->useContexts.end(), context) != item->useContexts.end();
            };

            switch (effect.type) {
                case ActiveItemEffectType::RerollOffers:
                    if (!hasContext(ActiveItemUseContext::Reward) &&
                        !hasContext(ActiveItemUseContext::Shop) &&
                        !hasContext(ActiveItemUseContext::Chest)) {
                        addError(errors, owner + " reroll_offers effect requires reward, shop, or chest context");
                    }
                    break;
                case ActiveItemEffectType::SkipEnemyTurn:
                    if (!hasContext(ActiveItemUseContext::Combat)) {
                        addError(errors, owner + " skip_enemy_turn effect requires combat context");
                    }
                    break;
                case ActiveItemEffectType::StabilizeStress:
                    if (!hasContext(ActiveItemUseContext::Combat)) {
                        addError(errors, owner + " stabilize_stress effect requires combat context");
                    }
                    break;
                case ActiveItemEffectType::CreateConsumable:
                    if (item->useContexts.size() == 1 && hasContext(ActiveItemUseContext::Combat)) {
                        addError(errors, owner + " create_consumable effect requires a non-combat context");
                    }
                    break;
                case ActiveItemEffectType::RerollMapChoices:
                    if (!hasContext(ActiveItemUseContext::Map)) {
                        addError(errors, owner + " reroll_map_choices effect requires map context");
                    }
                    break;
                case ActiveItemEffectType::CopyCard:
                    if (!hasContext(ActiveItemUseContext::Reward) && !hasContext(ActiveItemUseContext::Shop)) {
                        addError(errors, owner + " copy_card effect requires reward or shop context");
                    }
                    break;
                case ActiveItemEffectType::HealParty:
                case ActiveItemEffectType::GainGold:
                    break;
            }
        }
    }

    for (const DroneDefinition* drone : content.drones().all()) {
        if (drone == nullptr) {
            continue;
        }

        const std::string owner = "Drone '" + drone->id.value + "'";
        validateTextReference(errors, localization, owner, "name", drone->nameTextId);
        validateTextReference(errors, localization, owner, "description", drone->descriptionTextId);
        if (drone->activeAction.has_value()) {
            if (!drone->activeAction->logTextId.empty()) {
                validateTextReference(errors, localization, owner + " active action", "log", TextId(drone->activeAction->logTextId));
            }
            validateEffectList(errors, content, owner + " active action", drone->activeAction->effects);
        }
        if (drone->passiveAction.has_value()) {
            if (!drone->passiveAction->logTextId.empty()) {
                validateTextReference(errors, localization, owner + " passive action", "log", TextId(drone->passiveAction->logTextId));
            }
            validateEffectList(errors, content, owner + " passive action", drone->passiveAction->effects);
        }
    }

    for (const EnemyDefinition* enemy : content.enemies().all()) {
        if (enemy == nullptr) {
            continue;
        }

        const std::string owner = "Enemy '" + enemy->id.value + "'";
        validateTextReference(errors, localization, owner, "name", enemy->nameTextId);
        for (const EnemyActionDefinition& action : enemy->actions) {
            const std::string actionOwner = owner + " action '" + action.id + "'";
            validateTextReference(
                errors,
                localization,
                actionOwner,
                "name",
                TextId("enemy.action." + action.id + ".name")
            );
            validateEffectList(
                errors,
                content,
                actionOwner,
                action.effects
            );
            validateEnemyActionAi(errors, content, actionOwner, action);
        }

        if (enemy->role == EnemyRole::Boss && enemy->phases.size() < 2) {
            addError(errors, owner + " is a boss but has fewer than two phases");
        }
        if (enemy->role != EnemyRole::Boss && !enemy->phases.empty()) {
            addError(errors, owner + " has boss phases but is not classified as a boss");
        }

        std::vector<std::string> phasedActionIds;
        for (const EnemyPhaseDefinition& phase : enemy->phases) {
            const std::string phaseOwner = owner + " phase '" + phase.id + "'";
            validateTextReference(errors, localization, phaseOwner, "name", phase.nameTextId);
            validateEffectList(errors, content, phaseOwner + " on-enter effects", phase.onEnterEffects);
            validateEffectList(errors, content, phaseOwner + " player-turn effects", phase.playerTurnEffects);

            for (const std::string& actionId : phase.actionIds) {
                const bool knownAction = std::any_of(
                    enemy->actions.begin(), enemy->actions.end(),
                    [&actionId](const EnemyActionDefinition& action) { return action.id == actionId; }
                );
                if (!knownAction) {
                    addError(errors, phaseOwner + " references unknown action '" + actionId + "'");
                }
                if (std::find(phasedActionIds.begin(), phasedActionIds.end(), actionId) == phasedActionIds.end()) {
                    phasedActionIds.push_back(actionId);
                }
            }

            for (const std::string& summonId : phase.summonEnemyIds) {
                if (!content.enemies().contains(EnemyId(summonId))) {
                    addError(errors, phaseOwner + " summons unknown enemy '" + summonId + "'");
                    continue;
                }
                if (content.enemies().get(EnemyId(summonId)).role == EnemyRole::Boss) {
                    addError(errors, phaseOwner + " must not summon another boss '" + summonId + "'");
                }
            }
        }

        for (const EnemyActionDefinition& action : enemy->actions) {
            if (!enemy->phases.empty() &&
                std::find(phasedActionIds.begin(), phasedActionIds.end(), action.id) == phasedActionIds.end()) {
                addError(errors, owner + " action '" + action.id + "' is unreachable from every phase");
            }
        }
    }

    std::unordered_map<std::string, int> statusExclusiveGroupSizes;
    for (const StatusDefinition* status : content.statuses().all()) {
        if (status == nullptr) {
            continue;
        }

        const std::string owner = "Status '" + status->id.value + "'";
        validateTextReference(errors, localization, owner, "name", status->nameTextId);
        validateTextReference(errors, localization, owner, "description", status->descriptionTextId);

        if (!status->exclusiveGroup.empty()) {
            ++statusExclusiveGroupSizes[status->exclusiveGroup];
        }

        for (std::size_t index = 0; index < status->modifiers.size(); ++index) {
            const StatusModifierDefinition& modifier = status->modifiers[index];
            const std::string modifierOwner = owner + " modifier[" + std::to_string(index) + "]";
            validateTextReference(
                errors,
                localization,
                modifierOwner,
                "description",
                modifier.descriptionTextId
            );

            switch (modifier.operation) {
                case StatusModifierOperation::AddPerStack:
                case StatusModifierOperation::AddFixed:
                    if (modifier.addAmount == 0) {
                        addError(errors, modifierOwner + " has a zero additive value");
                    }
                    break;

                case StatusModifierOperation::MultiplyPerStack:
                    if (modifier.multiplier == 0.0) {
                        addError(errors, modifierOwner + " has a zero per-stack multiplier");
                    }
                    break;

                case StatusModifierOperation::MultiplyFixed:
                    if (modifier.multiplier <= 0.0) {
                        addError(errors, modifierOwner + " has a non-positive fixed multiplier");
                    }
                    break;
            }
        }

        for (std::size_t index = 0; index < status->triggers.size(); ++index) {
            const StatusTriggerDefinition& trigger = status->triggers[index];
            const std::string triggerOwner = owner + " trigger[" + std::to_string(index) + "]";
            if (trigger.flatValue == 0 && trigger.valuePerStack == 0) {
                addError(errors, triggerOwner + " has no effect value");
            }
            if (trigger.removeStacks < 0) {
                addError(errors, triggerOwner + " removes a negative number of stacks");
            }
            if (trigger.logType != StatusTriggerLogType::None &&
                trigger.effect != StatusTriggeredEffect::DamageHp) {
                addError(errors, triggerOwner + " uses a damage log for a non-damage effect");
            }
        }

        if (status->durationRule == StatusDurationRule::Custom && status->triggers.empty()) {
            addError(errors, owner + " uses custom duration without any triggers");
        }
    }

    for (const auto& [groupId, size] : statusExclusiveGroupSizes) {
        if (size < 2) {
            addError(errors, "Status exclusive group '" + groupId + "' contains fewer than two statuses");
        }
    }

    for (const RelicDefinition* relic : content.relics().all()) {
        if (relic == nullptr) {
            continue;
        }

        const std::string owner = "Relic '" + relic->id.value + "'";
        validateTextReference(errors, localization, owner, "name", relic->nameTextId);
        validateTextReference(errors, localization, owner, "description", relic->descriptionTextId);
        for (const RelicTriggerDefinition& trigger : relic->triggers) {
            if (trigger.statusId.has_value() && !content.statuses().contains(StatusId(*trigger.statusId))) {
                addError(errors, owner + " trigger references unknown status filter '" + *trigger.statusId + "'");
            }
            if (trigger.cardType.has_value() && trigger.eventType != GameEventType::CardPlayed) {
                addError(errors, owner + " trigger has card_type filter, but card_type is currently supported only for card_played events");
            }
            if (trigger.previousCardType.has_value() && trigger.eventType != GameEventType::CardPlayed) {
                addError(errors, owner + " trigger has previous_card_type filter outside card_played event");
            }
            if (trigger.cardNumberThisTurn > 0 && trigger.eventType != GameEventType::CardPlayed) {
                addError(errors, owner + " trigger has card_number_this_turn outside card_played event");
            }
            if (trigger.cardNumberThisTurn < 0) {
                addError(errors, owner + " trigger has negative card_number_this_turn");
            }
            if (trigger.ownerStatusId.has_value() && !content.statuses().contains(StatusId(*trigger.ownerStatusId))) {
                addError(errors, owner + " trigger references unknown owner status '" + *trigger.ownerStatusId + "'");
            }
            if (trigger.minimumDrones < 0) {
                addError(errors, owner + " trigger has negative min_drones");
            }
            if (trigger.breakdownType.has_value() && trigger.eventType != GameEventType::StressBreakdownTriggered) {
                addError(errors, owner + " trigger has breakdown_type filter outside stress_breakdown_triggered event");
            }
            if (trigger.minimumBreakdownSeverity > 0 && trigger.eventType != GameEventType::StressBreakdownTriggered) {
                addError(errors, owner + " trigger has min_breakdown_severity outside stress_breakdown_triggered event");
            }
            if (trigger.minimumBreakdownSeverity < 0) {
                addError(errors, owner + " trigger has negative min_breakdown_severity");
            }
            if (trigger.minimumAmount < 0) {
                addError(errors, owner + " trigger has negative min_amount");
            }
            if (trigger.sourceSide != "any" && trigger.sourceSide != "player" && trigger.sourceSide != "enemy") {
                addError(errors, owner + " trigger has invalid source_side '" + trigger.sourceSide + "'");
            }
            validateEffectList(errors, content, owner + " trigger", trigger.effects);
        }
    }

    for (const RunEventDefinition* event : content.events().all()) {
        if (event == nullptr) {
            continue;
        }

        const std::string eventOwner = "Event '" + event->id + "'";
        validateTextReference(errors, localization, eventOwner, "title", event->titleTextId);
        validateTextReference(errors, localization, eventOwner, "description", event->descriptionTextId);

        for (std::size_t choiceIndex = 0; choiceIndex < event->choices.size(); ++choiceIndex) {
            const RunEventChoiceDefinition& choice = event->choices[choiceIndex];
            const std::string owner = eventOwner + " choice " + std::to_string(choiceIndex);

            validateTextReference(errors, localization, owner, "text", choice.textTextId);
            validateTextReference(errors, localization, owner, "description", choice.descriptionTextId);

            for (const std::string& relicId : choice.requirements.requiredRelicIds) {
                if (!content.relics().contains(RelicId(relicId))) {
                    addError(errors, owner + " requires unknown relic '" + relicId + "'");
                }
            }

            for (const std::string& relicId : choice.requirements.forbiddenRelicIds) {
                if (!content.relics().contains(RelicId(relicId))) {
                    addError(errors, owner + " forbids unknown relic '" + relicId + "'");
                }
            }

            for (const std::string& cardId : choice.requirements.requiredCardIds) {
                if (!content.cards().contains(CardId(cardId))) {
                    addError(errors, owner + " requires unknown card '" + cardId + "'");
                }
            }

            for (const std::string& cardId : choice.requirements.forbiddenCardIds) {
                if (!content.cards().contains(CardId(cardId))) {
                    addError(errors, owner + " forbids unknown card '" + cardId + "'");
                }
            }

            if (choice.requirements.maxStress > 0 && choice.requirements.minStress > choice.requirements.maxStress) {
                addError(errors, owner + " has min_stress greater than max_stress");
            }
            const auto validStressTrait = [](const std::string& traitId) {
                return traitId == "stress_breakdown" || traitId == "stress_resolve";
            };
            for (const std::string& traitId : choice.requirements.requiredTraitIds) {
                if (!validStressTrait(traitId)) {
                    addError(errors, owner + " requires unknown trait '" + traitId + "'");
                }
            }
            for (const std::string& traitId : choice.requirements.forbiddenTraitIds) {
                if (!validStressTrait(traitId)) {
                    addError(errors, owner + " forbids unknown trait '" + traitId + "'");
                }
            }

            for (std::size_t effectIndex = 0; effectIndex < choice.effects.size(); ++effectIndex) {
                validateRunEventEffect(
                    errors,
                    content,
                    owner + " effect " + std::to_string(effectIndex),
                    choice.effects[effectIndex]
                );
            }
        }
    }



    for (const AchievementDefinition* achievement : content.achievements().all()) {
        if (achievement == nullptr) {
            continue;
        }

        const std::string owner = "Achievement '" + achievement->id + "'";
        validateTextReference(errors, localization, owner, "name", achievement->nameTextId);
        validateTextReference(errors, localization, owner, "description", achievement->descriptionTextId);
        validateTextReference(errors, localization, owner, "goal", achievement->goalTextId);
        validateTextReference(errors, localization, owner, "reward", achievement->rewardTextId);
        validateTextReference(errors, localization, owner, "unlock_hint", achievement->unlockHintTextId);
        validateUnlockReward(errors, content, owner, achievement->reward);

        for (const std::string& archetypeId : achievement->requiredUnlockedArchetypeIds) {
            if (!content.archetypes().contains(PlayableArchetypeId(archetypeId))) {
                addError(errors, owner + " requires unknown unlocked archetype '" + archetypeId + "'");
            }
        }

        for (const std::string& requiredChallengeId : achievement->requiredCompletedChallengeIds) {
            if (!content.challenges().contains(requiredChallengeId)) {
                addError(errors, owner + " requires unknown completed challenge '" + requiredChallengeId + "'");
            }
        }

        for (const std::string& requiredAchievementId : achievement->requiredCompletedAchievementIds) {
            if (!content.achievements().contains(requiredAchievementId)) {
                addError(errors, owner + " requires unknown completed achievement '" + requiredAchievementId + "'");
            }
            if (requiredAchievementId == achievement->id) {
                addError(errors, owner + " cannot require itself");
            }
        }

        const AchievementCompletionCondition& completion = achievement->completion;
        if (completion.type != "clear_floor" &&
            completion.type != "clear_floor_with_elites" &&
            completion.type != "clear_floor_with_archetype" &&
            completion.type != "clear_floor_low_damage" &&
            completion.type != "complete_challenges" &&
            completion.type != "complete_achievements" &&
            completion.type != "win_runs" &&
            completion.type != "lose_runs") {
            addError(errors, owner + " has unknown completion type '" + completion.type + "'");
        }

        if (!completion.floorId.empty() && !content.floors().contains(completion.floorId)) {
            addError(errors, owner + " references unknown completion floor '" + completion.floorId + "'");
        }

        if (!completion.archetypeId.empty() && !content.archetypes().contains(PlayableArchetypeId(completion.archetypeId))) {
            addError(errors, owner + " references unknown completion archetype '" + completion.archetypeId + "'");
        }

        if (completion.minVictories < 0 ||
            completion.minDefeats < 0 ||
            completion.minCompletedChallenges < 0 ||
            completion.minCompletedAchievements < 0 ||
            completion.minElitesKilled < 0 ||
            completion.minBossesKilled < 0 ||
            completion.minEventsCompleted < 0 ||
            completion.minRelics < 0 ||
            completion.minGold < 0) {
            addError(errors, owner + " has negative minimum counters");
        }

        if (completion.maxDamageTaken < -1) {
            addError(errors, owner + " has invalid max damage below -1");
        }
    }

    for (const FloorDefinition* floor : content.floors().all()) {
        if (floor == nullptr) {
            continue;
        }

        validateTextReference(errors, localization, "Floor '" + floor->id + "'", "name", TextId(floor->nameTextId));

        if (!floor->isImplemented) {
            continue;
        }

        const EncounterDatabase& encounters = content.encountersForFloor(floor->id);
        for (const EncounterDefinition* encounter : encounters.all()) {
            if (encounter == nullptr) {
                continue;
            }

            if (encounter->enemyIds.empty()) {
                addError(errors, "Floor '" + floor->id + "' encounter '" + encounter->id + "' has no enemies");
            }
            if (encounter->enemyIds.size() > EncounterDefinition::MaximumEnemyCount) {
                addError(
                    errors,
                    "Floor '" + floor->id + "' encounter '" + encounter->id + "' exceeds the supported enemy limit of " +
                    std::to_string(EncounterDefinition::MaximumEnemyCount)
                );
            }
            for (const std::string& enemyId : encounter->enemyIds) {
                if (!content.enemies().contains(EnemyId(enemyId))) {
                    addError(errors, "Floor '" + floor->id + "' encounter '" + encounter->id + "' references unknown enemy '" + enemyId + "'");
                }
            }
        }
    }

    for (const PlayableArchetypeDefinition* archetype : content.archetypes().all()) {
        if (archetype == nullptr) {
            continue;
        }

        const std::string owner = "Archetype '" + archetype->id.value + "'";
        validateTextReference(errors, localization, owner, "name", archetype->nameTextId);
        validateTextReference(errors, localization, owner, "short_description", archetype->shortDescriptionTextId);
        validateTextReference(errors, localization, owner, "details_description", archetype->detailsDescriptionTextId);
        validateTextReference(errors, localization, owner, "unique_mechanic", archetype->uniqueMechanicTextId);
        validateTextReference(errors, localization, owner, "visual_identity", archetype->visualIdentityTextId);
        validateTextReference(errors, localization, owner, "palette.name", archetype->palette.nameTextId);
        validateTextReferenceList(errors, localization, owner, "strengths", archetype->strengthTextIds);
        validateTextReferenceList(errors, localization, owner, "weaknesses", archetype->weaknessTextIds);

        for (const std::string& actorId : archetype->actorDefinitionIds) {
            if (!content.actors().contains(PlayerActorId(actorId))) {
                addError(errors, owner + " references unknown actor '" + actorId + "'");
            }
        }

        if (archetype->rewardCardPoolIds.empty()) {
            addError(errors, owner + " must define at least one reward card pool");
        }

        for (const std::string& poolId : archetype->rewardCardPoolIds) {
            const bool knownActorPool = content.actors().contains(PlayerActorId(poolId));
            bool knownSharedCardPool = false;
            for (const CardDefinition* card : content.cards().all()) {
                if (card != nullptr && rewardPoolKey(*card) == poolId) {
                    knownSharedCardPool = true;
                    break;
                }
            }

            if (!knownActorPool && !knownSharedCardPool) {
                addError(errors, owner + " references unknown reward card pool '" + poolId + "'");
            }
        }

        if (archetype->isAvailable && !hasArchetypeCardRewardCandidates(content, archetype->rewardCardPoolIds)) {
            addError(errors, owner + " has no non-starter card reward candidates in its reward card pools");
        }

        for (const std::string& cardId : archetype->startingDeckCardIds) {
            if (!content.cards().contains(CardId(cardId))) {
                addError(errors, owner + " references unknown starting card '" + cardId + "'");
            }
        }

        for (const std::string& relicId : archetype->startingRelicIds) {
            if (!content.relics().contains(RelicId(relicId))) {
                addError(errors, owner + " references unknown starting relic '" + relicId + "'");
            }
        }

        for (const std::string& consumableId : archetype->startingConsumableIds) {
            if (!content.consumables().contains(ConsumableId(consumableId))) {
                addError(errors, owner + " references unknown starting consumable '" + consumableId + "'");
            }
        }
    }

    for (const PlayerActorDefinition* actor : content.actors().all()) {
        if (actor == nullptr) {
            continue;
        }

        const std::string owner = "Actor '" + actor->id.value + "'";
        validateTextReference(errors, localization, owner, "name", actor->nameTextId);
        validateTextReference(errors, localization, owner, "description", actor->descriptionTextId);
        for (const std::string& relicId : actor->startingRelicIds) {
            if (!content.relics().contains(RelicId(relicId))) {
                addError(errors, owner + " references unknown starting relic '" + relicId + "'");
            }
        }
    }

    for (const DifficultyDefinition* difficulty : content.difficulties().all()) {
        if (difficulty == nullptr) {
            continue;
        }

        const std::string owner = "Difficulty '" + difficulty->id.value + "'";
        validateTextReference(errors, localization, owner, "name", difficulty->nameTextId);
        validateTextReference(errors, localization, owner, "description", difficulty->descriptionTextId);
    }

    validateRewardTuning(errors, content);
    validateShopTuning(errors, content);
    validateMapGeneration(errors, content, localization);



    for (const AchievementDefinition* achievement : content.achievements().all()) {
        if (achievement == nullptr) {
            continue;
        }

        const std::string owner = "Achievement '" + achievement->id + "'";
        validateTextReference(errors, localization, owner, "name", achievement->nameTextId);
        validateTextReference(errors, localization, owner, "description", achievement->descriptionTextId);
        validateTextReference(errors, localization, owner, "goal", achievement->goalTextId);
        validateTextReference(errors, localization, owner, "reward", achievement->rewardTextId);
        validateTextReference(errors, localization, owner, "unlock_hint", achievement->unlockHintTextId);
        validateUnlockReward(errors, content, owner, achievement->reward);

        for (const std::string& archetypeId : achievement->requiredUnlockedArchetypeIds) {
            if (!content.archetypes().contains(PlayableArchetypeId(archetypeId))) {
                addError(errors, owner + " requires unknown unlocked archetype '" + archetypeId + "'");
            }
        }

        for (const std::string& requiredChallengeId : achievement->requiredCompletedChallengeIds) {
            if (!content.challenges().contains(requiredChallengeId)) {
                addError(errors, owner + " requires unknown completed challenge '" + requiredChallengeId + "'");
            }
        }

        for (const std::string& requiredAchievementId : achievement->requiredCompletedAchievementIds) {
            if (!content.achievements().contains(requiredAchievementId)) {
                addError(errors, owner + " requires unknown completed achievement '" + requiredAchievementId + "'");
            }
            if (requiredAchievementId == achievement->id) {
                addError(errors, owner + " cannot require itself");
            }
        }

        const AchievementCompletionCondition& completion = achievement->completion;
        if (completion.type != "clear_floor" &&
            completion.type != "clear_floor_with_elites" &&
            completion.type != "clear_floor_with_archetype" &&
            completion.type != "clear_floor_low_damage" &&
            completion.type != "complete_challenges" &&
            completion.type != "complete_achievements" &&
            completion.type != "win_runs" &&
            completion.type != "lose_runs") {
            addError(errors, owner + " has unknown completion type '" + completion.type + "'");
        }

        if (!completion.floorId.empty() && !content.floors().contains(completion.floorId)) {
            addError(errors, owner + " references unknown completion floor '" + completion.floorId + "'");
        }

        if (!completion.archetypeId.empty() && !content.archetypes().contains(PlayableArchetypeId(completion.archetypeId))) {
            addError(errors, owner + " references unknown completion archetype '" + completion.archetypeId + "'");
        }

        if (completion.minVictories < 0 ||
            completion.minDefeats < 0 ||
            completion.minCompletedChallenges < 0 ||
            completion.minCompletedAchievements < 0 ||
            completion.minElitesKilled < 0 ||
            completion.minBossesKilled < 0 ||
            completion.minEventsCompleted < 0 ||
            completion.minRelics < 0 ||
            completion.minGold < 0) {
            addError(errors, owner + " has negative minimum counters");
        }

        if (completion.maxDamageTaken < -1) {
            addError(errors, owner + " has invalid max damage below -1");
        }
    }

    for (const FloorDefinition* floor : content.floors().all()) {
        if (floor == nullptr || !floor->isImplemented) {
            continue;
        }

        validateFloorContent(
            errors,
            content,
            *floor,
            content.mapGenerationForFloor(floor->id),
            content.encountersForFloor(floor->id)
        );
    }

    if (!errors.empty()) {
        throw std::runtime_error(joinErrors(errors));
    }
}
}

void ContentValidator::validate(const ContentRegistry& content) {
    validateContent(content, nullptr);
}

void ContentValidator::validate(const ContentRegistry& content, const LocalizationManager& localization) {
    validateContent(content, &localization);
}
