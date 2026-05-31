#include "ContentValidator.hpp"

#include "cards/CardId.hpp"
#include "consumables/ConsumableId.hpp"
#include "drones/DroneId.hpp"
#include "effects/EffectDefinition.hpp"
#include "enemies/EnemyId.hpp"
#include "localization/Locale.hpp"
#include "localization/LocalizationManager.hpp"
#include "localization/TextId.hpp"
#include "relics/RelicId.hpp"
#include "rewards/RewardPoolRules.hpp"
#include "run/RunMapNode.hpp"
#include "statuses/StatusId.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
void addError(std::vector<std::string>& errors, std::string message) {
    errors.push_back(std::move(message));
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

    const StatusId statusId(*effect.statusId);
    if (!content.statuses().contains(statusId)) {
        addError(errors, owner + " references unknown status '" + *effect.statusId + "'");
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

        if (card->ownerActorId.empty()) {
            continue;
        }

        if (std::find(rewardCardPoolIds.begin(), rewardCardPoolIds.end(), card->ownerActorId) != rewardCardPoolIds.end()) {
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
        case RunEventEffectType::GainRandomCard:
            if (!hasCardRewardCandidates(content)) {
                addError(errors, owner + " can generate a random card, but the card reward pool is empty");
            }
            break;
        case RunEventEffectType::GainRandomRelic:
            if (!hasRelicRewardCandidates(content)) {
                addError(errors, owner + " can generate a random relic, but the relic reward pool is empty");
            }
            break;
        case RunEventEffectType::GainRandomConsumable:
            if (!hasConsumableCandidates(content)) {
                addError(errors, owner + " can generate a random consumable, but the consumable pool is empty");
            }
            break;
        case RunEventEffectType::Skip:
            if (effect.amount != 0) {
                addError(errors, owner + " has skip event effect with a non-zero amount");
            }
            break;
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

        if (tuning.guaranteedRelic && !relicRewardPoolAvailable) {
            addError(errors, owner + " guarantees a relic, but the relic reward pool is empty");
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
    const ContentRegistry& content
) {
    const RunMapGenerationConfig& config = content.actOneMapGeneration();
    const bool canGenerateEventNodes =
        config.eventWeight() > 0 ||
        (config.hasFixedEvents() && config.events().maximum > 0);
    const bool canResolveQuestionMarksToEvents = config.questionMarkCombatChance() < 100;

    if (canGenerateEventNodes && canResolveQuestionMarksToEvents && content.events().all().empty()) {
        addError(errors, "Act '" + config.id() + "' can generate event nodes, but no run events are loaded");
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
        }
    }

    for (const StatusDefinition* status : content.statuses().all()) {
        if (status == nullptr) {
            continue;
        }

        const std::string owner = "Status '" + status->id.value + "'";
        validateTextReference(errors, localization, owner, "name", status->nameTextId);
        validateTextReference(errors, localization, owner, "description", status->descriptionTextId);

        if (!status->endTurnEffect.empty()) {
            if (status->endTurnEffect != "poison_damage") {
                addError(errors, owner + " has unknown end_turn_effect '" + status->endTurnEffect + "'");
            }

            validateTextReference(
                errors,
                localization,
                owner,
                "end_turn_effect",
                TextId("status.end_turn_effect." + status->endTurnEffect)
            );
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

    for (const EncounterDefinition* encounter : content.encounters().all()) {
        if (encounter == nullptr) {
            continue;
        }

        for (const std::string& enemyId : encounter->enemyIds) {
            if (!content.enemies().contains(EnemyId(enemyId))) {
                addError(errors, "Encounter '" + encounter->id + "' references unknown enemy '" + enemyId + "'");
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
            if (!content.actors().contains(PlayerActorId(poolId))) {
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
    validateMapGeneration(errors, content);

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
