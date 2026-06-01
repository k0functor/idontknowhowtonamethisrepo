#include "events/RunEventRequirement.hpp"

#include <algorithm>

namespace {
bool containsId(const std::vector<std::string>& ids, const std::string& id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

bool deckContainsCardId(const std::vector<CardId>& ids, const std::string& id) {
    return std::find(ids.begin(), ids.end(), CardId(id)) != ids.end();
}

int totalCurrentHp(const RunState& state) {
    int total = 0;
    for (const RunActorState& actor : state.actorStates) {
        total += std::max(0, actor.currentHp);
    }
    return total;
}
}

RunEventChoiceAvailability evaluateRunEventChoiceRequirements(
    const RunEventChoiceRequirements& requirements,
    const RunState& state
) {
    RunEventChoiceAvailability result;

    if (requirements.minGold > 0 && state.gold < requirements.minGold) {
        result.reasons.push_back(RunEventChoiceBlockReason{
            RunEventChoiceBlockReasonType::NotEnoughGold,
            requirements.minGold,
            state.gold,
            {}
        });
    }

    const int currentHp = totalCurrentHp(state);
    if (requirements.minHp > 0 && currentHp < requirements.minHp) {
        result.reasons.push_back(RunEventChoiceBlockReason{
            RunEventChoiceBlockReasonType::NotEnoughHp,
            requirements.minHp,
            currentHp,
            {}
        });
    }

    if (requirements.freeConsumableSlot && static_cast<int>(state.consumableIds.size()) >= state.maxConsumables) {
        result.reasons.push_back(RunEventChoiceBlockReason{
            RunEventChoiceBlockReasonType::NoFreeConsumableSlot,
            state.maxConsumables,
            static_cast<int>(state.consumableIds.size()),
            {}
        });
    }

    if (requirements.minDeckSize > 0 && static_cast<int>(state.deckCardIds.size()) < requirements.minDeckSize) {
        result.reasons.push_back(RunEventChoiceBlockReason{
            RunEventChoiceBlockReasonType::NotEnoughCards,
            requirements.minDeckSize,
            static_cast<int>(state.deckCardIds.size()),
            {}
        });
    }

    for (const std::string& cardId : requirements.requiredCardIds) {
        if (!deckContainsCardId(state.deckCardIds, cardId)) {
            result.reasons.push_back(RunEventChoiceBlockReason{
                RunEventChoiceBlockReasonType::MissingRequiredCard,
                0,
                0,
                cardId
            });
        }
    }

    for (const std::string& cardId : requirements.forbiddenCardIds) {
        if (deckContainsCardId(state.deckCardIds, cardId)) {
            result.reasons.push_back(RunEventChoiceBlockReason{
                RunEventChoiceBlockReasonType::HasForbiddenCard,
                0,
                0,
                cardId
            });
        }
    }

    for (const std::string& relicId : requirements.requiredRelicIds) {
        if (!containsId(state.relicIds, relicId)) {
            result.reasons.push_back(RunEventChoiceBlockReason{
                RunEventChoiceBlockReasonType::MissingRequiredRelic,
                0,
                0,
                relicId
            });
        }
    }

    for (const std::string& relicId : requirements.forbiddenRelicIds) {
        if (containsId(state.relicIds, relicId)) {
            result.reasons.push_back(RunEventChoiceBlockReason{
                RunEventChoiceBlockReasonType::HasForbiddenRelic,
                0,
                0,
                relicId
            });
        }
    }

    result.available = result.reasons.empty();
    return result;
}
