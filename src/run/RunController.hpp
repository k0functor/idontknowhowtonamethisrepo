#pragma once

#include <cstddef>

#include "archetypes/PlayableArchetypeDefinition.hpp"
#include "actors/PlayerActorDatabase.hpp"
#include "combat/CombatResult.hpp"
#include "cards/CardId.hpp"
#include "core/Random.hpp"
#include "data/CardDatabase.hpp"
#include "consumables/ConsumableDatabase.hpp"
#include "events/RunEventDefinition.hpp"
#include "rewards/RewardGenerator.hpp"
#include "rewards/RewardSelection.hpp"
#include "rewards/RewardState.hpp"
#include "rewards/RewardTuning.hpp"
#include "rewards/RewardSystem.hpp"
#include "relics/RelicDatabase.hpp"
#include "relics/RelicId.hpp"
#include "shop/ShopOffer.hpp"
#include "run/DifficultyDefinition.hpp"
#include "run/FloorDefinition.hpp"
#include "run/RunFactory.hpp"
#include "run/RunMapNode.hpp"
#include "run/RunMapGenerationConfig.hpp"
#include "run/RunState.hpp"

#include <cstdint>
#include <optional>
#include <string>

class RunController {
public:
    void startNewRun(
        const PlayableArchetypeDefinition& archetype,
        const DifficultyDefinition& difficulty,
        const PlayerActorDatabase& actors,
        const RunMapGenerationConfig& mapGeneration,
        const FloorDefinition& floor,
        std::uint32_t seed
    );

    bool hasActiveRun() const;
    void restoreRun(RunState run);
    void clearActiveRun();

    Random& random();
    void syncRandomStateToRun();

    bool isActCompleted() const;
    void completeCurrentAct();
    bool advanceToNextFloor(
        const FloorDefinition& floor,
        const RunMapGenerationConfig& mapGeneration
    );

    const RunState& run() const;
    RunState& run();

    bool hasPendingRoom() const;
    bool hasPendingRoomForNode(int nodeId) const;
    bool pendingRoomMatchesRunMap() const;
    bool partyDefeated() const;
    const RunPendingRoomState& pendingRoom() const;
    void clearPendingRoom();
    void setPendingCombatReward(int nodeId, RewardState reward);
    void setPendingChestReward(int nodeId, RewardState reward);
    void setPendingShop(int nodeId, ShopState shop);
    void setPendingMerchantRest(int nodeId, ShopState shop);
    void setPendingEvent(int nodeId, std::string eventId);

    const RunMapNode& node(int nodeId) const;
    RunMapNode& node(int nodeId);
    void revealNodeType(int nodeId, RunMapNodeType type);

    bool canStartNode(int nodeId) const;
    bool canCompleteNode(int nodeId) const;
    bool canUseRestNode(int nodeId) const;
    void startNode(int nodeId);

    void completeCombat(
        int nodeId,
        const CombatResult& combatResult
    );
    void recordCombatDefeat(const CombatResult& combatResult);

    RewardState completeCombatAndCreateReward(
        int nodeId,
        const CombatResult& combatResult,
        const CardDatabase& cards,
        const RelicDatabase& relics,
        const ConsumableDatabase& consumables,
        const RewardTuning& rewardTuning,
        Random& random
    );

    RewardState completeCombatAndCreateReward(
        int nodeId,
        const CardDatabase& cards,
        const RelicDatabase& relics,
        const ConsumableDatabase& consumables,
        const RewardTuning& rewardTuning,
        Random& random
    );

    void applyReward(
        const RewardState& reward,
        const RewardSelection& selection
    );

    void healAllActorsByPercent(float percent);
    void healAllActorsFlat(int amount);
    void damageAllActorsNonlethal(int amount);
    void reduceAllActorsStress(int amount);
    void adjustAllActorsStress(int delta, Random* random = nullptr);

    std::optional<RelicId> chooseChestRelic(
        const RelicDatabase& relics,
        Random& random
    ) const;

    void completeChestAndTakeRelic(int nodeId, const RelicId& relicId);
    void completeChestNode(int nodeId);
    void completeEventNode(int nodeId);
    void completeRestHeal(int nodeId);
    bool completeRestUpgrade(int nodeId, std::size_t deckIndex);
    void completeRestSkip(int nodeId);

    bool purchaseShopItem(const ShopPurchase& purchase);
    void completeShopNode(int nodeId);
    void completeMerchantRestNode(int nodeId);

    bool completeEventChoice(
        int nodeId,
        const RunEventChoiceDefinition& choice,
        const CardDatabase& cards,
        const RelicDatabase& relics,
        const ConsumableDatabase& consumables,
        Random& random
    );

private:
    void markNodeCompletedAndUnlockNext(int nodeId);

private:
    RunFactory runFactory_;
    RewardGenerator rewardGenerator_;
    RewardSystem rewardSystem_;
    std::optional<RunState> activeRun_;
    std::optional<Random> activeRunRandom_;
};
