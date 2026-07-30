#include "ActiveItemContextSystem.hpp"

#include "run/RunCardEligibility.hpp"

#include <array>
#include <vector>

namespace {
bool compassCanChange(const RunMapNodeType type) {
    switch (type) {
        case RunMapNodeType::Combat:
        case RunMapNodeType::Event:
        case RunMapNodeType::Shop:
        case RunMapNodeType::Chest:
            return true;
        case RunMapNodeType::Elite:
        case RunMapNodeType::Rest:
        case RunMapNodeType::Boss:
            return false;
    }
    return false;
}

const CardDefinition* findCard(const CardDatabase& cards, const CardId& cardId) {
    for (const CardDefinition* card : cards.all()) {
        if (card != nullptr && card->id == cardId) {
            return card;
        }
    }
    return nullptr;
}
}

std::optional<std::string> ActiveItemContextSystem::createRandomConsumable(
    RunState& run,
    const ConsumableDatabase& consumables,
    Random& random
) {
    if (run.maxConsumables <= 0 || static_cast<int>(run.consumableIds.size()) >= run.maxConsumables) {
        return std::nullopt;
    }

    std::vector<const ConsumableDefinition*> candidates;
    for (const ConsumableDefinition* consumable : consumables.all()) {
        if (consumable != nullptr) {
            candidates.push_back(consumable);
        }
    }
    if (candidates.empty()) {
        return std::nullopt;
    }

    const std::size_t index = static_cast<std::size_t>(
        random.rangeInclusive(0, static_cast<int>(candidates.size()) - 1)
    );
    const std::string id = candidates[index]->id.value;
    run.consumableIds.push_back(id);
    ++run.stats.consumablesGained;
    return id;
}

int ActiveItemContextSystem::rerollAvailableMapNodes(RunState& run, Random& random) {
    constexpr std::array<RunMapNodeType, 4> candidates{
        RunMapNodeType::Combat,
        RunMapNodeType::Event,
        RunMapNodeType::Shop,
        RunMapNodeType::Chest
    };

    int changed = 0;
    for (RunMapNode& node : run.map.nodes) {
        if (node.state != RunMapNodeState::Available || !compassCanChange(node.type)) {
            continue;
        }

        std::vector<RunMapNodeType> alternatives;
        for (const RunMapNodeType candidate : candidates) {
            if (candidate != node.type) {
                alternatives.push_back(candidate);
            }
        }
        if (alternatives.empty()) {
            continue;
        }

        const std::size_t index = static_cast<std::size_t>(
            random.rangeInclusive(0, static_cast<int>(alternatives.size()) - 1)
        );
        node.type = alternatives[index];
        ++changed;
    }
    return changed;
}

bool ActiveItemContextSystem::copyCard(
    RunState& run,
    const CardDatabase& cards,
    const CardId& cardId
) {
    const CardDefinition* card = findCard(cards, cardId);
    if (card == nullptr || !runCanReceiveCard(run, *card)) {
        return false;
    }

    run.deckCardIds.push_back(cardId);
    ++run.stats.cardsAdded;
    return true;
}
