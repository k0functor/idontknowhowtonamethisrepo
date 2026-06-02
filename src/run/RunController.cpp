#include "RunController.hpp"

#include "cards/CardRarity.hpp"
#include "cards/CardType.hpp"
#include "consumables/ConsumableDefinition.hpp"
#include "consumables/ConsumableId.hpp"
#include "relics/RelicRarity.hpp"
#include "rewards/RewardPoolRules.hpp"
#include "run/RunCardEligibility.hpp"
#include "run/StressRules.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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

bool runAlreadyHasRelic(const RunState& state, const std::string& relicId) {
    return std::find(state.relicIds.begin(), state.relicIds.end(), relicId) != state.relicIds.end();
}

bool addSpecificCardToRun(RunState& state, const CardDatabase& cards, const std::string& cardId) {
    if (cardId.empty() || !cards.contains(CardId(cardId))) {
        return false;
    }

    state.deckCardIds.push_back(CardId(cardId));
    ++state.stats.cardsAdded;
    return true;
}

bool addSpecificRelicToRun(RunState& state, const RelicDatabase& relics, const std::string& relicId) {
    if (relicId.empty() || !relics.contains(RelicId(relicId)) || runAlreadyHasRelic(state, relicId)) {
        return false;
    }

    state.relicIds.push_back(relicId);
    ++state.stats.relicsGained;
    return true;
}

bool addSpecificConsumableToRun(RunState& state, const ConsumableDatabase& consumables, const std::string& consumableId) {
    if (consumableId.empty() ||
        !consumables.contains(ConsumableId(consumableId)) ||
        static_cast<int>(state.consumableIds.size()) >= state.maxConsumables) {
        return false;
    }

    state.consumableIds.push_back(consumableId);
    ++state.stats.consumablesGained;
    return true;
}


bool containsDeckIndex(const std::vector<int>& indices, const std::size_t deckIndex) {
    if (deckIndex > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return false;
    }

    const int value = static_cast<int>(deckIndex);
    return std::find(indices.begin(), indices.end(), value) != indices.end();
}


int combatDamageTaken(const std::vector<RunActorState>& before, const std::vector<RunActorState>& after) {
    int damage = 0;
    std::vector<bool> matched(before.size(), false);

    for (std::size_t afterIndex = 0; afterIndex < after.size(); ++afterIndex) {
        const RunActorState& current = after[afterIndex];
        std::optional<std::size_t> beforeIndex;

        for (std::size_t index = 0; index < before.size(); ++index) {
            if (!matched[index] && before[index].definitionId == current.definitionId) {
                beforeIndex = index;
                break;
            }
        }

        if (!beforeIndex.has_value() && afterIndex < before.size() && !matched[afterIndex]) {
            beforeIndex = afterIndex;
        }

        if (!beforeIndex.has_value()) {
            continue;
        }

        matched[*beforeIndex] = true;
        damage += std::max(0, before[*beforeIndex].currentHp - current.currentHp);
    }

    return damage;
}

int combatConsumablesUsed(
    const std::vector<std::string>& before,
    const std::optional<std::vector<std::string>>& after
) {
    if (!after.has_value()) {
        return 0;
    }

    return std::max(0, static_cast<int>(before.size()) - static_cast<int>(after->size()));
}

void recordCombatTelemetry(RunState& state, const CombatResult& combatResult) {
    state.stats.enemiesKilled += combatResult.enemiesKilled;

    if (!combatResult.actorStates.empty()) {
        state.stats.damageTaken += combatDamageTaken(state.actorStates, combatResult.actorStates);
        state.actorStates = combatResult.actorStates;
        state.actorDefinitionIds.clear();
        state.actorDefinitionIds.reserve(state.actorStates.size());
        for (const RunActorState& actorState : state.actorStates) {
            state.actorDefinitionIds.push_back(actorState.definitionId);
        }
    }

    if (combatResult.remainingConsumableIds.has_value()) {
        state.stats.consumablesUsed += combatConsumablesUsed(state.consumableIds, combatResult.remainingConsumableIds);
        state.consumableIds = *combatResult.remainingConsumableIds;
    }
}

bool eraseDeckIndexAndShiftUpgrades(RunState& state, const std::size_t erasedIndex) {
    if (erasedIndex >= state.deckCardIds.size()) {
        return false;
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
    return true;
}


bool pendingRoomUsesCompletedNode(const RunPendingRoomState& pending) {
    return pending.type == RunPendingRoomType::CombatReward;
}

bool pendingRoomTypeMatchesNodeType(const RunPendingRoomState& pending, const RunMapNodeType nodeType) {
    switch (pending.type) {
        case RunPendingRoomType::CombatReward:
            return nodeType == RunMapNodeType::Combat ||
                nodeType == RunMapNodeType::Elite ||
                nodeType == RunMapNodeType::Boss;
        case RunPendingRoomType::ChestReward:
            return nodeType == RunMapNodeType::Chest;
        case RunPendingRoomType::Shop:
            return nodeType == RunMapNodeType::Shop;
        case RunPendingRoomType::MerchantRest:
            return nodeType == RunMapNodeType::Rest;
        case RunPendingRoomType::Event:
            return nodeType == RunMapNodeType::Event;
        case RunPendingRoomType::None:
            return true;
    }

    return false;
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
    activeRunRandom_.emplace(activeRun_->seed);
    activeRunRandom_->setState(activeRun_->randomState);
}

bool RunController::hasActiveRun() const {
    return activeRun_.has_value();
}

void RunController::restoreRun(RunState run) {
    activeRun_ = std::move(run);
    activeRunRandom_.emplace(activeRun_->seed);
    if (!activeRun_->randomState.empty()) {
        activeRunRandom_->setState(activeRun_->randomState);
    } else {
        activeRun_->randomState = activeRunRandom_->state();
    }
}

void RunController::clearActiveRun() {
    activeRun_.reset();
    activeRunRandom_.reset();
}

Random& RunController::random() {
    if (!activeRunRandom_.has_value()) {
        throw std::runtime_error("No active run random generator");
    }

    return *activeRunRandom_;
}

void RunController::syncRandomStateToRun() {
    if (activeRun_.has_value() && activeRunRandom_.has_value()) {
        activeRun_->randomState = activeRunRandom_->state();
    }
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

bool RunController::hasPendingRoomForNode(const int nodeId) const {
    return hasPendingRoom() && run().pendingRoom.nodeId == nodeId;
}


bool RunController::pendingRoomMatchesRunMap() const {
    if (!hasPendingRoom()) {
        return true;
    }

    const RunPendingRoomState& pending = run().pendingRoom;
    const RunMapNode* pendingNode = nullptr;
    for (const RunMapNode& nodeCandidate : run().map.nodes) {
        if (nodeCandidate.id == pending.nodeId) {
            pendingNode = &nodeCandidate;
            break;
        }
    }

    if (pendingNode == nullptr) {
        return false;
    }

    if (!pendingRoomTypeMatchesNodeType(pending, pendingNode->type)) {
        return false;
    }

    const RunMapNodeState expectedState = pendingRoomUsesCompletedNode(pending)
        ? RunMapNodeState::Completed
        : RunMapNodeState::Current;
    return pendingNode->state == expectedState;
}

bool RunController::partyDefeated() const {
    if (!hasActiveRun() || run().actorStates.empty()) {
        return false;
    }

    return std::all_of(
        run().actorStates.begin(),
        run().actorStates.end(),
        [](const RunActorState& actor) {
            return actor.currentHp <= 0;
        }
    );
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

void RunController::setPendingMerchantRest(const int nodeId, ShopState shop) {
    RunPendingRoomState pending;
    pending.type = RunPendingRoomType::MerchantRest;
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

bool RunController::canCompleteNode(const int nodeId) const {
    const RunMapNode& target = node(nodeId);
    return target.state == RunMapNodeState::Current || target.state == RunMapNodeState::Completed;
}

bool RunController::canUseRestNode(const int nodeId) const {
    const RunMapNode& target = node(nodeId);
    return target.type == RunMapNodeType::Rest && canStartNode(nodeId);
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

void RunController::completeCombat(
    const int nodeId,
    const CombatResult& combatResult
) {
    const RunMapNodeType completedNodeType = node(nodeId).type;

    RunState& state = run();
    recordCombatTelemetry(state, combatResult);

    markNodeCompletedAndUnlockNext(nodeId);

    ++state.stats.combatsWon;

    if (completedNodeType == RunMapNodeType::Elite) {
        ++state.stats.elitesKilled;
    } else if (completedNodeType == RunMapNodeType::Boss) {
        ++state.stats.bossesKilled;
        state.defeatedBossEnemyIds = combatResult.killedEnemyIds;
    }
}

void RunController::recordCombatDefeat(const CombatResult& combatResult) {
    RunState& state = run();
    recordCombatTelemetry(state, combatResult);
    ++state.stats.combatsLost;
}

RewardState RunController::completeCombatAndCreateReward(
    const int nodeId,
    const CombatResult& combatResult,
    const CardDatabase& cards,
    const RelicDatabase& relics,
    const ConsumableDatabase& consumables,
    const RewardTuning& rewardTuning,
    Random& random
) {
    const RunMapNodeType completedNodeType = node(nodeId).type;
    completeCombat(nodeId, combatResult);

    return rewardGenerator_.generateCombatReward(
        RewardContext{run(), completedNodeType},
        cards,
        relics,
        consumables,
        rewardTuning,
        random
    );
}

RewardState RunController::completeCombatAndCreateReward(
    const int nodeId,
    const CardDatabase& cards,
    const RelicDatabase& relics,
    const ConsumableDatabase& consumables,
    const RewardTuning& rewardTuning,
    Random& random
) {
    return completeCombatAndCreateReward(
        nodeId,
        CombatResult{},
        cards,
        relics,
        consumables,
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
        ++state.stats.relicsGained;
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
    if (node(nodeId).type != RunMapNodeType::Rest) {
        throw std::runtime_error("Cannot heal at a non-rest map node: " + std::to_string(nodeId));
    }

    healAllActorsByPercent(0.30f);
    reduceAllActorsStress(30);
    ++run().stats.restHealsUsed;
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

bool RunController::completeRestUpgrade(const int nodeId, const std::size_t deckIndex) {
    if (node(nodeId).type != RunMapNodeType::Rest) {
        throw std::runtime_error("Cannot upgrade a card at a non-rest map node: " + std::to_string(nodeId));
    }

    RunState& state = run();
    if (deckIndex >= state.deckCardIds.size() || containsDeckIndex(state.upgradedDeckIndices, deckIndex)) {
        return false;
    }

    state.upgradedDeckIndices.push_back(static_cast<int>(deckIndex));
    std::sort(state.upgradedDeckIndices.begin(), state.upgradedDeckIndices.end());
    state.upgradedDeckIndices.erase(
        std::unique(state.upgradedDeckIndices.begin(), state.upgradedDeckIndices.end()),
        state.upgradedDeckIndices.end()
    );
    ++state.stats.cardsUpgraded;
    ++state.stats.restUpgradesUsed;

    markNodeCompletedAndUnlockNext(nodeId);
    return true;
}


void RunController::completeRestSkip(const int nodeId) {
    if (node(nodeId).type != RunMapNodeType::Rest) {
        throw std::runtime_error("Cannot skip rest at a non-rest map node: " + std::to_string(nodeId));
    }

    ++run().stats.restSkips;
    markNodeCompletedAndUnlockNext(nodeId);
}

bool RunController::purchaseShopItem(const ShopPurchase& purchase) {
    RunState& state = run();

    if (purchase.price < 0 || state.gold < purchase.price) {
        return false;
    }

    if (state.pendingRoom.type == RunPendingRoomType::MerchantRest) {
        const ShopState& merchantRest = state.pendingRoom.shop;
        if (purchase.type != ShopOfferType::Card || merchantRest.cardPurchasesRemaining() <= 0) {
            return false;
        }
    }

    switch (purchase.type) {
        case ShopOfferType::Card:
            if (purchase.contentId.empty()) {
                return false;
            }
            state.gold -= purchase.price;
            state.stats.goldSpent += purchase.price;
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
            state.stats.goldSpent += purchase.price;
            state.relicIds.push_back(purchase.contentId);
            ++state.stats.relicsGained;
            return true;
        }

        case ShopOfferType::Consumable:
            if (purchase.contentId.empty() || static_cast<int>(state.consumableIds.size()) >= state.maxConsumables) {
                return false;
            }
            state.gold -= purchase.price;
            state.stats.goldSpent += purchase.price;
            state.consumableIds.push_back(purchase.contentId);
            ++state.stats.consumablesGained;
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
            state.stats.goldSpent += purchase.price;
            if (eraseDeckIndexAndShiftUpgrades(state, removedIndex)) {
                ++state.stats.cardsRemoved;
            }
            return true;
        }
    }

    return false;
}

void RunController::completeShopNode(const int nodeId) {
    markNodeCompletedAndUnlockNext(nodeId);
}

void RunController::completeMerchantRestNode(const int nodeId) {
    if (node(nodeId).type != RunMapNodeType::Rest) {
        throw std::runtime_error("Cannot complete merchant rest at a non-rest map node: " + std::to_string(nodeId));
    }

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

            case RunEventEffectType::GainCard:
                addSpecificCardToRun(state, cards, effect.contentId);
                break;

            case RunEventEffectType::GainRandomCard: {
                const std::optional<CardId> card = chooseRandomCard(state, cards, random);
                if (card.has_value()) {
                    state.deckCardIds.push_back(*card);
                    ++state.stats.cardsAdded;
                }
                break;
            }

            case RunEventEffectType::GainRelic:
                addSpecificRelicToRun(state, relics, effect.contentId);
                break;

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
                        ++state.stats.relicsGained;
                    }
                }
                break;
            }

            case RunEventEffectType::GainConsumable:
                addSpecificConsumableToRun(state, consumables, effect.contentId);
                break;

            case RunEventEffectType::GainRandomConsumable: {
                if (static_cast<int>(state.consumableIds.size()) < state.maxConsumables) {
                    const std::optional<std::string> consumable = chooseRandomConsumable(consumables, random);
                    if (consumable.has_value()) {
                        state.consumableIds.push_back(*consumable);
                        ++state.stats.consumablesGained;
                    }
                }
                break;
            }

            case RunEventEffectType::RemoveCard: {
                const auto iterator = std::find(state.deckCardIds.begin(), state.deckCardIds.end(), CardId(effect.contentId));
                if (iterator != state.deckCardIds.end()) {
                    if (eraseDeckIndexAndShiftUpgrades(
                        state,
                        static_cast<std::size_t>(std::distance(state.deckCardIds.begin(), iterator))
                    )) {
                        ++state.stats.cardsRemoved;
                    }
                }
                break;
            }

            case RunEventEffectType::RemoveRandomCard:
                if (!state.deckCardIds.empty()) {
                    const int index = random.rangeInclusive(0, static_cast<int>(state.deckCardIds.size()) - 1);
                    if (eraseDeckIndexAndShiftUpgrades(state, static_cast<std::size_t>(index))) {
                        ++state.stats.cardsRemoved;
                    }
                }
                break;

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

        if (current.state == RunMapNodeState::Completed) {
            map.currentNodeId = nodeId;
            run().pendingRoom.clear();
            return;
        }

        if (current.state != RunMapNodeState::Current) {
            throw std::runtime_error(
                "Cannot complete run map node that is not current: " + std::to_string(nodeId)
            );
        }

        current.state = RunMapNodeState::Completed;
        map.currentNodeId = nodeId;

        RunStats& stats = run().stats;
        ++stats.nodesCompleted;
        switch (current.type) {
            case RunMapNodeType::Event:
                ++stats.eventsCompleted;
                break;

            case RunMapNodeType::Shop:
                ++stats.shopsVisited;
                break;

            case RunMapNodeType::Chest:
                ++stats.chestsOpened;
                break;

            case RunMapNodeType::Rest:
                ++stats.restsUsed;
                break;

            case RunMapNodeType::Combat:
            case RunMapNodeType::Elite:
            case RunMapNodeType::Boss:
                break;
        }

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
