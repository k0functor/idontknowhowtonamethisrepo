#include "RunController.hpp"

#include "relics/RelicRarity.hpp"
#include "cards/CardRarity.hpp"
#include "cards/CardType.hpp"
#include "consumables/ConsumableDefinition.hpp"
#include "consumables/ConsumableId.hpp"
#include "rewards/RewardPoolRules.hpp"
#include "run/RunCardEligibility.hpp"
#include "run/StressRules.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>
#include <limits>
#include <optional>
#include <utility>


namespace {
bool canAppearAsGeneratedCard(const CardDefinition& card) {
    if (card.type == CardType::Status || card.type == CardType::Curse) {
        return false;
    }

    if (card.rarity == CardRarity::Starter || card.rarity == CardRarity::Special) {
        return false;
    }

    return true;
}

std::optional<CardId> chooseRandomCard(const RunState& run, const CardDatabase& cards, Random& random) {
    std::vector<const CardDefinition*> candidates;

    for (const CardDefinition* card : cards.all()) {
        if (card != nullptr && canAppearAsGeneratedCard(*card) && runCanReceiveCard(run, *card)) {
            candidates.push_back(card);
        }
    }

    if (candidates.empty()) {
        return std::nullopt;
    }

    const int index = random.rangeInclusive(0, static_cast<int>(candidates.size()) - 1);
    return candidates[static_cast<std::size_t>(index)]->id;
}

std::optional<std::string> chooseRandomConsumable(const ConsumableDatabase& consumables, Random& random) {
    std::vector<const ConsumableDefinition*> candidates;

    for (const ConsumableDefinition* consumable : consumables.all()) {
        if (consumable != nullptr) {
            candidates.push_back(consumable);
        }
    }

    if (candidates.empty()) {
        return std::nullopt;
    }

    const int index = random.rangeInclusive(0, static_cast<int>(candidates.size()) - 1);
    return candidates[static_cast<std::size_t>(index)]->id.value;
}


bool containsDeckIndex(const std::vector<int>& indices, const std::size_t deckIndex) {
    if (deckIndex > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return false;
    }

    const int value = static_cast<int>(deckIndex);
    return std::find(indices.begin(), indices.end(), value) != indices.end();
}

void eraseDeckIndexAndShiftUpgrades(RunState& state, const std::size_t erasedIndex) {
    if (erasedIndex >= state.deckCardIds.size()) {
        return;
    }

    state.deckCardIds.erase(state.deckCardIds.begin() + static_cast<std::ptrdiff_t>(erasedIndex));

    std::vector<int> updated;
    updated.reserve(state.upgradedDeckIndices.size());
    for (const int index : state.upgradedDeckIndices) {
        if (index < 0) {
            continue;
        }

        const std::size_t current = static_cast<std::size_t>(index);
        if (current == erasedIndex) {
            continue;
        }

        updated.push_back(current > erasedIndex ? index - 1 : index);
    }

    std::sort(updated.begin(), updated.end());
    updated.erase(std::unique(updated.begin(), updated.end()), updated.end());
    state.upgradedDeckIndices = std::move(updated);
}

}

void RunController::startNewRun(
    const PlayableArchetypeDefinition& archetype,
    const DifficultyDefinition& difficulty,
    const PlayerActorDatabase& actors,
    const RunMapGenerationConfig& mapGeneration,
    const std::uint32_t seed
) {
    activeRun_ = runFactory_.createRun(archetype, difficulty, actors, mapGeneration, seed);
}

bool RunController::hasActiveRun() const {
    return activeRun_.has_value();
}

void RunController::restoreRun(RunState run) {
    activeRun_ = std::move(run);
}

void RunController::clearActiveRun() {
    activeRun_.reset();
}

bool RunController::isActCompleted() const {
    return hasActiveRun() && run().actCompleted;
}

void RunController::completeCurrentAct() {
    RunState& state = run();
    state.actCompleted = true;
    state.completedAct = state.act;
    state.pendingRoom.clear();
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

bool RunController::hasPendingRoom() const {
    return hasActiveRun() && run().pendingRoom.active();
}

const RunPendingRoomState& RunController::pendingRoom() const {
    return run().pendingRoom;
}

void RunController::clearPendingRoom() {
    run().pendingRoom.clear();
}

void RunController::setPendingCombatReward(const int nodeId, RewardState reward) {
    RunPendingRoomState pending;
    pending.type = RunPendingRoomType::CombatReward;
    pending.nodeId = nodeId;
    pending.reward = std::move(reward);
    run().pendingRoom = std::move(pending);
}

void RunController::setPendingChestReward(const int nodeId, RewardState reward) {
    RunPendingRoomState pending;
    pending.type = RunPendingRoomType::ChestReward;
    pending.nodeId = nodeId;
    pending.reward = std::move(reward);
    run().pendingRoom = std::move(pending);
}

void RunController::setPendingShop(const int nodeId, ShopState shop) {
    RunPendingRoomState pending;
    pending.type = RunPendingRoomType::Shop;
    pending.nodeId = nodeId;
    pending.shop = std::move(shop);
    run().pendingRoom = std::move(pending);
}

void RunController::setPendingEvent(const int nodeId, std::string eventId) {
    RunPendingRoomState pending;
    pending.type = RunPendingRoomType::Event;
    pending.nodeId = nodeId;
    pending.eventId = std::move(eventId);
    run().pendingRoom = std::move(pending);
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

void RunController::revealNodeType(const int nodeId, const RunMapNodeType type) {
    node(nodeId).type = type;
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
    const RewardTuning& rewardTuning,
    Random& random
) {
    const RunMapNodeType completedNodeType = node(nodeId).type;

    RunState& state = run();
    if (!combatResult.actorStates.empty()) {
        state.actorStates = combatResult.actorStates;
        state.actorDefinitionIds.clear();
        state.actorDefinitionIds.reserve(state.actorStates.size());
        for (const RunActorState& actorState : state.actorStates) {
            state.actorDefinitionIds.push_back(actorState.definitionId);
        }
    }

    if (combatResult.remainingConsumableIds.has_value()) {
        state.consumableIds = *combatResult.remainingConsumableIds;
    }

    markNodeCompletedAndUnlockNext(nodeId);

    ++state.stats.combatsWon;

    if (completedNodeType == RunMapNodeType::Elite) {
        ++state.stats.elitesKilled;
    } else if (completedNodeType == RunMapNodeType::Boss) {
        ++state.stats.bossesKilled;
        state.defeatedBossEnemyIds = combatResult.killedEnemyIds;
    }

    return rewardGenerator_.generateCombatReward(
        RewardContext{state, completedNodeType},
        cards,
        relics,
        rewardTuning,
        random
    );
}

RewardState RunController::completeCombatAndCreateReward(
    const int nodeId,
    const CardDatabase& cards,
    const RelicDatabase& relics,
    const RewardTuning& rewardTuning,
    Random& random
) {
    return completeCombatAndCreateReward(
        nodeId,
        CombatResult{},
        cards,
        relics,
        rewardTuning,
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

        if (!RewardPoolRules::canAppearAsRelicReward(*relic)) {
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
    RunState& state = run();
    const bool alreadyOwned = std::find(
        state.relicIds.begin(),
        state.relicIds.end(),
        relicId.value
    ) != state.relicIds.end();

    if (!alreadyOwned) {
        state.relicIds.push_back(relicId.value);
    }

    markNodeCompletedAndUnlockNext(nodeId);
}

void RunController::completeChestNode(const int nodeId) {
    markNodeCompletedAndUnlockNext(nodeId);
}

void RunController::completeEventNode(const int nodeId) {
    // Event rooms are a placeholder for now. They still consume the route choice.
    markNodeCompletedAndUnlockNext(nodeId);
}

void RunController::completeRestHeal(const int nodeId) {
    healAllActorsByPercent(0.30f);
    reduceAllActorsStress(30);
    markNodeCompletedAndUnlockNext(nodeId);
}

void RunController::healAllActorsByPercent(const float percent) {
    RunState& state = run();
    for (RunActorState& actor : state.actorStates) {
        if (actor.maxHp <= 0) {
            actor.maxHp = std::max(1, actor.currentHp);
        }

        const int amount = std::max(1, static_cast<int>(static_cast<float>(actor.maxHp) * percent + 0.5f));
        actor.currentHp = std::clamp(actor.currentHp + amount, 0, actor.maxHp);
    }
}

void RunController::healAllActorsFlat(const int amount) {
    if (amount <= 0) {
        return;
    }

    RunState& state = run();
    for (RunActorState& actor : state.actorStates) {
        if (actor.maxHp <= 0) {
            actor.maxHp = std::max(1, actor.currentHp);
        }

        actor.currentHp = std::clamp(actor.currentHp + amount, 0, actor.maxHp);
    }
}

void RunController::damageAllActorsNonlethal(const int amount) {
    if (amount <= 0) {
        return;
    }

    RunState& state = run();
    for (RunActorState& actor : state.actorStates) {
        if (actor.currentHp <= 0) {
            continue;
        }

        actor.currentHp = std::max(1, actor.currentHp - amount);
    }
}

void RunController::reduceAllActorsStress(const int amount) {
    if (amount <= 0) {
        return;
    }

    adjustAllActorsStress(-amount);
}

void RunController::adjustAllActorsStress(const int delta, Random* random) {
    RunState& state = run();
    for (RunActorState& actor : state.actorStates) {
        const StressRules::StressAdjustmentResult result = StressRules::applyDelta(actor, delta, random);
        if (result.collapsed) {
            actor.currentHp = 0;
        }
    }
}

void RunController::completeRestUpgrade(const int nodeId, const std::size_t deckIndex) {
    RunState& state = run();

    if (deckIndex < state.deckCardIds.size() && !containsDeckIndex(state.upgradedDeckIndices, deckIndex)) {
        state.upgradedDeckIndices.push_back(static_cast<int>(deckIndex));
        std::sort(state.upgradedDeckIndices.begin(), state.upgradedDeckIndices.end());
        state.upgradedDeckIndices.erase(
            std::unique(state.upgradedDeckIndices.begin(), state.upgradedDeckIndices.end()),
            state.upgradedDeckIndices.end()
        );
    }

    markNodeCompletedAndUnlockNext(nodeId);
}


void RunController::completeRestSkip(const int nodeId) {
    markNodeCompletedAndUnlockNext(nodeId);
}

bool RunController::purchaseShopItem(const ShopPurchase& purchase) {
    RunState& state = run();

    if (purchase.price < 0 || state.gold < purchase.price) {
        return false;
    }

    switch (purchase.type) {
        case ShopOfferType::Card:
            if (purchase.contentId.empty()) {
                return false;
            }
            state.gold -= purchase.price;
            state.deckCardIds.push_back(CardId(purchase.contentId));
            ++state.stats.cardsAdded;
            return true;

        case ShopOfferType::Relic: {
            if (purchase.contentId.empty()) {
                return false;
            }

            const bool alreadyOwned = std::find(
                state.relicIds.begin(),
                state.relicIds.end(),
                purchase.contentId
            ) != state.relicIds.end();

            if (alreadyOwned) {
                return false;
            }

            state.gold -= purchase.price;
            state.relicIds.push_back(purchase.contentId);
            return true;
        }

        case ShopOfferType::Consumable:
            if (purchase.contentId.empty() || static_cast<int>(state.consumableIds.size()) >= state.maxConsumables) {
                return false;
            }
            state.gold -= purchase.price;
            state.consumableIds.push_back(purchase.contentId);
            return true;

        case ShopOfferType::CardRemoval: {
            std::size_t removedIndex = 0;

            if (purchase.hasDeckIndex) {
                if (purchase.deckIndex >= state.deckCardIds.size()) {
                    return false;
                }

                removedIndex = purchase.deckIndex;
            } else {
                const auto iterator = std::find(state.deckCardIds.begin(), state.deckCardIds.end(), purchase.cardId);
                if (iterator == state.deckCardIds.end()) {
                    return false;
                }

                removedIndex = static_cast<std::size_t>(std::distance(state.deckCardIds.begin(), iterator));
            }

            state.gold -= purchase.price;
            eraseDeckIndexAndShiftUpgrades(state, removedIndex);
            return true;
        }
    }

    return false;
}

void RunController::completeShopNode(const int nodeId) {
    markNodeCompletedAndUnlockNext(nodeId);
}

bool RunController::completeEventChoice(
    const int nodeId,
    const RunEventChoiceDefinition& choice,
    const CardDatabase& cards,
    const RelicDatabase& relics,
    const ConsumableDatabase& consumables,
    Random& random
) {
    RunState& state = run();

    const RunEventChoiceAvailability availability = evaluateRunEventChoiceRequirements(choice.requirements, state);
    if (!availability.available) {
        return false;
    }

    for (const RunEventEffect& effect : choice.effects) {
        switch (effect.type) {
            case RunEventEffectType::GainGold:
                if (effect.amount > 0) {
                    state.gold += effect.amount;
                    state.stats.goldGained += effect.amount;
                }
                break;

            case RunEventEffectType::LoseGold:
                if (effect.amount > 0) {
                    state.gold = std::max(0, state.gold - effect.amount);
                }
                break;

            case RunEventEffectType::GainRandomCard: {
                const std::optional<CardId> card = chooseRandomCard(state, cards, random);
                if (card.has_value()) {
                    state.deckCardIds.push_back(*card);
                    ++state.stats.cardsAdded;
                }
                break;
            }

            case RunEventEffectType::GainRandomRelic: {
                const std::optional<RelicId> relic = chooseChestRelic(relics, random);
                if (relic.has_value()) {
                    const bool alreadyOwned = std::find(
                        state.relicIds.begin(),
                        state.relicIds.end(),
                        relic->value
                    ) != state.relicIds.end();
                    if (!alreadyOwned) {
                        state.relicIds.push_back(relic->value);
                    }
                }
                break;
            }

            case RunEventEffectType::GainRandomConsumable: {
                if (static_cast<int>(state.consumableIds.size()) < state.maxConsumables) {
                    const std::optional<std::string> consumable = chooseRandomConsumable(consumables, random);
                    if (consumable.has_value()) {
                        state.consumableIds.push_back(*consumable);
                    }
                }
                break;
            }

            case RunEventEffectType::GainStress:
                adjustAllActorsStress(std::max(0, effect.amount), &random);
                break;

            case RunEventEffectType::LoseStress:
                reduceAllActorsStress(std::max(0, effect.amount));
                break;

            case RunEventEffectType::LoseHp:
                damageAllActorsNonlethal(std::max(0, effect.amount));
                break;

            case RunEventEffectType::HealAll:
                healAllActorsFlat(std::max(0, effect.amount));
                break;

            case RunEventEffectType::Skip:
                break;
        }
    }

    markNodeCompletedAndUnlockNext(nodeId);
    return true;
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

        run().pendingRoom.clear();
        return;
    }

    throw std::runtime_error("Cannot complete unknown run map node: " + std::to_string(nodeId));
}
