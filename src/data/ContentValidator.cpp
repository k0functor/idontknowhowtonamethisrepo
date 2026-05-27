#include "ContentValidator.hpp"

#include "cards/CardId.hpp"
#include "consumables/ConsumableId.hpp"
#include "drones/DroneId.hpp"
#include "effects/EffectDefinition.hpp"
#include "relics/RelicId.hpp"
#include "statuses/StatusId.hpp"

#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
void addError(std::vector<std::string>& errors, std::string message) {
    errors.push_back(std::move(message));
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
}

void ContentValidator::validate(const ContentRegistry& content) {
    std::vector<std::string> errors;

    for (const CardDefinition* card : content.cards().all()) {
        if (card == nullptr) {
            continue;
        }

        const std::string owner = "Card '" + card->id.value + "'";
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

        validateEffectList(
            errors,
            content,
            "Consumable '" + consumable->id.value + "'",
            consumable->effects
        );
    }


    for (const DroneDefinition* drone : content.drones().all()) {
        if (drone == nullptr) {
            continue;
        }

        const std::string owner = "Drone '" + drone->id.value + "'";
        if (drone->manualAction.has_value()) {
            validateEffectList(errors, content, owner + " manual action", drone->manualAction->effects);
        }
        if (drone->endTurnAction.has_value()) {
            validateEffectList(errors, content, owner + " end-turn action", drone->endTurnAction->effects);
        }
    }

    for (const EnemyDefinition* enemy : content.enemies().all()) {
        if (enemy == nullptr) {
            continue;
        }

        for (const EnemyActionDefinition& action : enemy->actions) {
            validateEffectList(
                errors,
                content,
                "Enemy '" + enemy->id.value + "' action '" + action.id + "'",
                action.effects
            );
        }
    }

    for (const PlayableArchetypeDefinition* archetype : content.archetypes().all()) {
        if (archetype == nullptr) {
            continue;
        }

        const std::string owner = "Archetype '" + archetype->id.value + "'";
        for (const std::string& actorId : archetype->actorDefinitionIds) {
            if (!content.actors().contains(PlayerActorId(actorId))) {
                addError(errors, owner + " references unknown actor '" + actorId + "'");
            }
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
        for (const std::string& relicId : actor->startingRelicIds) {
            if (!content.relics().contains(RelicId(relicId))) {
                addError(errors, owner + " references unknown starting relic '" + relicId + "'");
            }
        }
    }

    if (!errors.empty()) {
        throw std::runtime_error(joinErrors(errors));
    }
}
