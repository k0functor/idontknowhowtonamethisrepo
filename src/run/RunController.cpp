#include "RunController.hpp"

#include <stdexcept>
#include <string>

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
