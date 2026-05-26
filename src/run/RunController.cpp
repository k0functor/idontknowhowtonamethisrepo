#include "RunController.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

void RunController::startNewRun(
    const PlayableArchetypeDefinition& archetype,
    const DifficultyDefinition& difficulty,
    const std::uint32_t seed
) {
    activeRun_ = runFactory_.createRun(archetype, difficulty, seed);
}

bool RunController::hasActiveRun() const {
    return activeRun_.has_value();
}

const RunState& RunController::run() const {
    if (!activeRun_.has_value()) {
        throw std::runtime_error("No active run");
    }

    return *activeRun_;
}

RunState& RunController::run() {
    if (!activeRun_.has_value()) {
        throw std::runtime_error("No active run");
    }

    return *activeRun_;
}

const RunMapNode& RunController::node(const int nodeId) const {
    const RunState& state = run();

    for (const RunMapNode& candidate : state.map.nodes) {
        if (candidate.id == nodeId) {
            return candidate;
        }
    }

    throw std::runtime_error("Unknown run map node id: " + std::to_string(nodeId));
}

RunMapNode& RunController::node(const int nodeId) {
    RunState& state = run();

    for (RunMapNode& candidate : state.map.nodes) {
        if (candidate.id == nodeId) {
            return candidate;
        }
    }

    throw std::runtime_error("Unknown run map node id: " + std::to_string(nodeId));
}

bool RunController::canStartNode(const int nodeId) const {
    const RunMapNode& target = node(nodeId);
    return target.state == RunMapNodeState::Available || target.state == RunMapNodeState::Current;
}

void RunController::startNode(const int nodeId) {
    if (!canStartNode(nodeId)) {
        throw std::runtime_error("Cannot start locked or completed run map node: " + std::to_string(nodeId));
    }

    RunMap& map = run().map;
    RunMapNode& selectedNode = node(nodeId);

    if (selectedNode.state == RunMapNodeState::Available) {
        for (RunMapNode& candidate : map.nodes) {
            if (candidate.id != nodeId && candidate.state == RunMapNodeState::Available) {
                candidate.state = RunMapNodeState::Locked;
            }
        }
    }

    map.currentNodeId = nodeId;
    selectedNode.state = RunMapNodeState::Current;
}

RewardState RunController::completeCombatAndCreateReward(
    const int nodeId,
    const CombatResult& combatResult,
    const CardDatabase& cards,
    const RelicDatabase& relics,
    Random& random
) {
    const RunMapNodeType completedNodeType = node(nodeId).type;

    (void)combatResult;

    markNodeCompletedAndUnlockNext(nodeId);

    RunState& state = run();
    ++state.stats.combatsWon;

    if (completedNodeType == RunMapNodeType::Elite) {
        ++state.stats.elitesKilled;
    } else if (completedNodeType == RunMapNodeType::Boss) {
        ++state.stats.bossesKilled;
    }

    return rewardGenerator_.generateCombatReward(
        RewardContext{state, completedNodeType},
        cards,
        relics,
        random
    );
}

RewardState RunController::completeCombatAndCreateReward(
    const int nodeId,
    const CardDatabase& cards,
    const RelicDatabase& relics,
    Random& random
) {
    return completeCombatAndCreateReward(
        nodeId,
        CombatResult{},
        cards,
        relics,
        random
    );
}

void RunController::applyReward(
    const RewardState& reward,
    const RewardSelection& selection
) {
    rewardSystem_.applyReward(run(), reward, selection);
}

std::optional<RelicId> RunController::chooseChestRelic(
    const RelicDatabase& relics,
    Random& random
) const {
    const RunState& state = run();
    std::vector<const RelicDefinition*> candidates;

    for (const RelicDefinition* relic : relics.all()) {
        if (relic == nullptr) {
            continue;
        }

        const bool alreadyOwned = std::find(
            state.relicIds.begin(),
            state.relicIds.end(),
            relic->id.value
        ) != state.relicIds.end();

        if (!alreadyOwned) {
            candidates.push_back(relic);
        }
    }

    if (candidates.empty()) {
        return std::nullopt;
    }

    const int index = random.rangeInclusive(0, static_cast<int>(candidates.size()) - 1);
    return candidates[static_cast<std::size_t>(index)]->id;
}

void RunController::completeChestAndTakeRelic(const int nodeId, const RelicId& relicId) {
    run().relicIds.push_back(relicId.value);
    markNodeCompletedAndUnlockNext(nodeId);
}

void RunController::completeEventNode(const int nodeId) {
    // Event rooms are a placeholder for now. They still consume the route choice.
    markNodeCompletedAndUnlockNext(nodeId);
}

void RunController::completeRestHeal(const int nodeId) {
    // Persistent actor HP is not fully modelled yet. This is the correct flow hook.
    markNodeCompletedAndUnlockNext(nodeId);
}

void RunController::completeRestUpgrade(const int nodeId) {
    RunState& state = run();

    for (const CardId& cardId : state.deckCardIds) {
        const bool alreadyUpgraded = std::find(
            state.upgradedCardIds.begin(),
            state.upgradedCardIds.end(),
            cardId
        ) != state.upgradedCardIds.end();

        if (!alreadyUpgraded) {
            state.upgradedCardIds.push_back(cardId);
            break;
        }
    }

    markNodeCompletedAndUnlockNext(nodeId);
}

void RunController::markNodeCompletedAndUnlockNext(const int nodeId) {
    RunMap& map = run().map;

    for (RunMapNode& current : map.nodes) {
        if (current.id != nodeId) {
            continue;
        }

        current.state = RunMapNodeState::Completed;
        map.currentNodeId = nodeId;
        ++run().stats.nodesCompleted;

        for (const int nextNodeId : current.nextNodeIds) {
            for (RunMapNode& candidate : map.nodes) {
                if (candidate.id == nextNodeId && candidate.state == RunMapNodeState::Locked) {
                    candidate.state = RunMapNodeState::Available;
                }
            }
        }

        return;
    }

    throw std::runtime_error("Cannot complete unknown run map node: " + std::to_string(nodeId));
}
