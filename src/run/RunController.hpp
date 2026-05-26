#pragma once

#include "archetypes/PlayableArchetypeDefinition.hpp"
#include "combat/CombatResult.hpp"
#include "cards/CardId.hpp"
#include "core/Random.hpp"
#include "data/CardDatabase.hpp"
#include "rewards/RewardGenerator.hpp"
#include "rewards/RewardSelection.hpp"
#include "rewards/RewardState.hpp"
#include "rewards/RewardSystem.hpp"
#include "relics/RelicDatabase.hpp"
#include "relics/RelicId.hpp"
#include "run/DifficultyDefinition.hpp"
#include "run/RunFactory.hpp"
#include "run/RunMapNode.hpp"
#include "run/RunState.hpp"

#include <cstdint>
#include <optional>

class RunController {
public:
    void startNewRun(
        const PlayableArchetypeDefinition& archetype,
        const DifficultyDefinition& difficulty,
        std::uint32_t seed
    );

    bool hasActiveRun() const;

    const RunState& run() const;
    RunState& run();

    const RunMapNode& node(int nodeId) const;
    RunMapNode& node(int nodeId);

    bool canStartNode(int nodeId) const;
    void startNode(int nodeId);

    RewardState completeCombatAndCreateReward(
        int nodeId,
        const CombatResult& combatResult,
        const CardDatabase& cards,
        const RelicDatabase& relics,
        Random& random
    );

    RewardState completeCombatAndCreateReward(
        int nodeId,
        const CardDatabase& cards,
        const RelicDatabase& relics,
        Random& random
    );

    void applyReward(
        const RewardState& reward,
        const RewardSelection& selection
    );

    std::optional<RelicId> chooseChestRelic(
        const RelicDatabase& relics,
        Random& random
    ) const;

    void completeChestAndTakeRelic(int nodeId, const RelicId& relicId);
    void completeEventNode(int nodeId);
    void completeRestHeal(int nodeId);
    void completeRestUpgrade(int nodeId);

private:
    void markNodeCompletedAndUnlockNext(int nodeId);

private:
    RunFactory runFactory_;
    RewardGenerator rewardGenerator_;
    RewardSystem rewardSystem_;
    std::optional<RunState> activeRun_;
};
