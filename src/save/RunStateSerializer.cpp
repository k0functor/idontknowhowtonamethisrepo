#include "RunStateSerializer.hpp"

#include "cards/CardId.hpp"
#include "core/Random.hpp"
#include "run/StressRules.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <raylib.h>

namespace {
[[noreturn]] void throwSaveError(const std::filesystem::path& sourcePath, const std::string& message) {
    throw std::runtime_error("Run save error in '" + sourcePath.string() + "': " + message);
}

const Json& requiredField(const Json& object, const std::string& key, const std::filesystem::path& sourcePath) {
    if (!object.is_object()) {
        throwSaveError(sourcePath, "Expected a JSON object");
    }

    if (!object.contains(key)) {
        throwSaveError(sourcePath, "Missing required field '" + key + "'");
    }

    return object.at(key);
}

const Json* optionalField(const Json& object, const std::string& key, const std::filesystem::path& sourcePath) {
    if (!object.is_object()) {
        throwSaveError(sourcePath, "Expected a JSON object");
    }

    if (!object.contains(key)) {
        return nullptr;
    }

    return &object.at(key);
}

std::string requiredString(const Json& object, const std::string& key, const std::filesystem::path& sourcePath) {
    const Json& value = requiredField(object, key, sourcePath);
    if (!value.is_string()) {
        throwSaveError(sourcePath, "'" + key + "' must be a string");
    }
    return value.get<std::string>();
}

int requiredInt(const Json& object, const std::string& key, const std::filesystem::path& sourcePath) {
    const Json& value = requiredField(object, key, sourcePath);
    if (!value.is_number_integer()) {
        throwSaveError(sourcePath, "'" + key + "' must be an integer");
    }
    return value.get<int>();
}

std::uint32_t requiredUnsigned(const Json& object, const std::string& key, const std::filesystem::path& sourcePath) {
    const Json& value = requiredField(object, key, sourcePath);
    if (!value.is_number_integer() && !value.is_number_unsigned()) {
        throwSaveError(sourcePath, "'" + key + "' must be an unsigned integer");
    }

    const long long number = value.get<long long>();
    if (number < 0) {
        throwSaveError(sourcePath, "'" + key + "' must be non-negative");
    }

    return static_cast<std::uint32_t>(number);
}

float requiredFloat(const Json& object, const std::string& key, const std::filesystem::path& sourcePath) {
    const Json& value = requiredField(object, key, sourcePath);
    if (!value.is_number()) {
        throwSaveError(sourcePath, "'" + key + "' must be a number");
    }
    return value.get<float>();
}

std::vector<std::string> requiredStringArray(const Json& object, const std::string& key, const std::filesystem::path& sourcePath) {
    const Json& array = requiredField(object, key, sourcePath);
    if (!array.is_array()) {
        throwSaveError(sourcePath, "'" + key + "' must be an array");
    }

    std::vector<std::string> result;
    result.reserve(array.size());

    for (std::size_t index = 0; index < array.size(); ++index) {
        const Json& value = array.at(index);
        if (!value.is_string()) {
            throwSaveError(sourcePath, "'" + key + "' element " + std::to_string(index) + " must be a string");
        }
        result.push_back(value.get<std::string>());
    }

    return result;
}

std::vector<CardId> requiredCardIdArray(const Json& object, const std::string& key, const std::filesystem::path& sourcePath) {
    std::vector<std::string> strings = requiredStringArray(object, key, sourcePath);
    std::vector<CardId> result;
    result.reserve(strings.size());
    for (const std::string& value : strings) {
        result.emplace_back(value);
    }
    return result;
}

std::vector<int> intArrayFromJson(const Json& array, const std::string& key, const std::filesystem::path& sourcePath) {
    if (!array.is_array()) {
        throwSaveError(sourcePath, "'" + key + "' must be an array");
    }

    std::vector<int> result;
    result.reserve(array.size());

    for (std::size_t index = 0; index < array.size(); ++index) {
        const Json& value = array.at(index);
        if (!value.is_number_integer()) {
            throwSaveError(sourcePath, "'" + key + "' element " + std::to_string(index) + " must be an integer");
        }
        result.push_back(value.get<int>());
    }

    return result;
}

std::vector<int> requiredIntArray(const Json& object, const std::string& key, const std::filesystem::path& sourcePath) {
    return intArrayFromJson(requiredField(object, key, sourcePath), key, sourcePath);
}

Json intArray(const std::vector<int>& values) {
    Json array = Json::array();
    for (const int value : values) {
        array.push_back(value);
    }
    return array;
}

std::vector<int> migrateLegacyUpgradedCardIds(
    const std::vector<CardId>& deckCardIds,
    const std::vector<CardId>& upgradedCardIds
) {
    std::vector<int> result;

    for (const CardId& upgradedCardId : upgradedCardIds) {
        for (std::size_t index = 0; index < deckCardIds.size(); ++index) {
            if (deckCardIds[index] == upgradedCardId &&
                std::find(result.begin(), result.end(), static_cast<int>(index)) == result.end()) {
                result.push_back(static_cast<int>(index));
                break;
            }
        }
    }

    std::sort(result.begin(), result.end());
    return result;
}

void normalizeUpgradedDeckIndices(RunState& run) {
    std::vector<int> normalized;
    normalized.reserve(run.upgradedDeckIndices.size());

    for (const int index : run.upgradedDeckIndices) {
        if (index >= 0 && static_cast<std::size_t>(index) < run.deckCardIds.size()) {
            normalized.push_back(index);
        }
    }

    std::sort(normalized.begin(), normalized.end());
    normalized.erase(std::unique(normalized.begin(), normalized.end()), normalized.end());
    run.upgradedDeckIndices = std::move(normalized);
}

Json stringArray(const std::vector<std::string>& values) {
    Json array = Json::array();
    for (const std::string& value : values) {
        array.push_back(value);
    }
    return array;
}

Json cardIdArray(const std::vector<CardId>& values) {
    Json array = Json::array();
    for (const CardId& value : values) {
        array.push_back(value.value);
    }
    return array;
}

std::string nodeTypeToString(const RunMapNodeType type) {
    switch (type) {
        case RunMapNodeType::Combat:
            return "combat";
        case RunMapNodeType::Elite:
            return "elite";
        case RunMapNodeType::Event:
            return "event";
        case RunMapNodeType::Shop:
            return "shop";
        case RunMapNodeType::Chest:
            return "chest";
        case RunMapNodeType::Rest:
            return "rest";
        case RunMapNodeType::Boss:
            return "boss";
    }

    return "combat";
}

RunMapNodeType nodeTypeFromString(const std::string& value, const std::filesystem::path& sourcePath) {
    if (value == "combat") {
        return RunMapNodeType::Combat;
    }
    if (value == "elite") {
        return RunMapNodeType::Elite;
    }
    if (value == "event") {
        return RunMapNodeType::Event;
    }
    if (value == "shop") {
        return RunMapNodeType::Shop;
    }
    if (value == "chest") {
        return RunMapNodeType::Chest;
    }
    if (value == "rest") {
        return RunMapNodeType::Rest;
    }
    if (value == "boss") {
        return RunMapNodeType::Boss;
    }

    throwSaveError(sourcePath, "Unknown run node type '" + value + "'");
}

std::string nodeStateToString(const RunMapNodeState state) {
    switch (state) {
        case RunMapNodeState::Locked:
            return "locked";
        case RunMapNodeState::Available:
            return "available";
        case RunMapNodeState::Completed:
            return "completed";
        case RunMapNodeState::Current:
            return "current";
    }

    return "locked";
}

RunMapNodeState nodeStateFromString(const std::string& value, const std::filesystem::path& sourcePath) {
    if (value == "locked") {
        return RunMapNodeState::Locked;
    }
    if (value == "available") {
        return RunMapNodeState::Available;
    }
    if (value == "completed") {
        return RunMapNodeState::Completed;
    }
    if (value == "current") {
        return RunMapNodeState::Current;
    }

    throwSaveError(sourcePath, "Unknown run node state '" + value + "'");
}

Json nodeToJson(const RunMapNode& node) {
    Json next = Json::array();
    for (const int id : node.nextNodeIds) {
        next.push_back(id);
    }

    return Json{
        {"id", node.id},
        {"type", nodeTypeToString(node.type)},
        {"state", nodeStateToString(node.state)},
        {"position", Json{{"x", node.position.x}, {"y", node.position.y}}},
        {"next", next}
    };
}

RunMapNode nodeFromJson(const Json& json, const std::filesystem::path& sourcePath) {
    RunMapNode node;
    node.id = requiredInt(json, "id", sourcePath);
    node.type = nodeTypeFromString(requiredString(json, "type", sourcePath), sourcePath);
    node.state = nodeStateFromString(requiredString(json, "state", sourcePath), sourcePath);

    const Json position = requiredField(json, "position", sourcePath);
    if (!position.is_object()) {
        throwSaveError(sourcePath, "'position' must be an object");
    }
    node.position = Vector2{
        requiredFloat(position, "x", sourcePath),
        requiredFloat(position, "y", sourcePath)
    };

    const Json next = requiredField(json, "next", sourcePath);
    if (!next.is_array()) {
        throwSaveError(sourcePath, "'next' must be an array");
    }
    for (std::size_t index = 0; index < next.size(); ++index) {
        const Json& value = next.at(index);
        if (!value.is_number_integer()) {
            throwSaveError(sourcePath, "'next' element " + std::to_string(index) + " must be an integer");
        }
        node.nextNodeIds.push_back(value.get<int>());
    }

    return node;
}

Json mapToJson(const RunMap& map) {
    Json nodes = Json::array();
    for (const RunMapNode& node : map.nodes) {
        nodes.push_back(nodeToJson(node));
    }

    return Json{
        {"current_node_id", map.currentNodeId},
        {"nodes", nodes}
    };
}

RunMap mapFromJson(const Json& json, const std::filesystem::path& sourcePath) {
    RunMap map;
    map.currentNodeId = requiredInt(json, "current_node_id", sourcePath);

    const Json nodes = requiredField(json, "nodes", sourcePath);
    if (!nodes.is_array()) {
        throwSaveError(sourcePath, "'nodes' must be an array");
    }

    map.nodes.reserve(nodes.size());
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        map.nodes.push_back(nodeFromJson(nodes.at(index), sourcePath));
    }

    return map;
}


Json actorStateToJson(const RunActorState& actor) {
    return Json{
        {"definition_id", actor.definitionId},
        {"current_hp", actor.currentHp},
        {"max_hp", actor.maxHp},
        {"stress", actor.stress},
        {"max_stress", actor.maxStress},
        {"resolve_check_triggered", actor.resolveCheckTriggered},
        {"trait_ids", stringArray(actor.traitIds)}
    };
}

RunActorState actorStateFromJson(const Json& json, const std::filesystem::path& sourcePath) {
    RunActorState actor;
    actor.definitionId = requiredString(json, "definition_id", sourcePath);
    actor.currentHp = requiredInt(json, "current_hp", sourcePath);
    actor.maxHp = requiredInt(json, "max_hp", sourcePath);

    if (const Json* stress = optionalField(json, "stress", sourcePath)) {
        if (!stress->is_number_integer()) {
            throwSaveError(sourcePath, "'stress' must be an integer");
        }
        actor.stress = stress->get<int>();
    }

    if (const Json* maxStress = optionalField(json, "max_stress", sourcePath)) {
        if (!maxStress->is_number_integer()) {
            throwSaveError(sourcePath, "'max_stress' must be an integer");
        }
        actor.maxStress = maxStress->get<int>();
    }

    if (const Json* resolveCheckTriggered = optionalField(json, "resolve_check_triggered", sourcePath)) {
        if (!resolveCheckTriggered->is_boolean()) {
            throwSaveError(sourcePath, "'resolve_check_triggered' must be a boolean");
        }
        actor.resolveCheckTriggered = resolveCheckTriggered->get<bool>();
    }

    if (const Json* traitIds = optionalField(json, "trait_ids", sourcePath)) {
        if (!traitIds->is_array()) {
            throwSaveError(sourcePath, "'trait_ids' must be an array");
        }

        for (std::size_t index = 0; index < traitIds->size(); ++index) {
            const Json& value = traitIds->at(index);
            if (!value.is_string()) {
                throwSaveError(sourcePath, "'trait_ids' element " + std::to_string(index) + " must be a string");
            }
            actor.traitIds.push_back(value.get<std::string>());
        }
    }

    if (actor.maxHp <= 0) {
        throwSaveError(sourcePath, "actor_states.max_hp must be positive");
    }
    if (actor.maxStress <= 0) {
        throwSaveError(sourcePath, "actor_states.max_stress must be positive");
    }

    actor.currentHp = std::clamp(actor.currentHp, 0, actor.maxHp);
    StressRules::normalize(actor);
    if (actor.stress >= actor.maxStress) {
        actor.currentHp = 0;
    }
    return actor;
}

Json actorStatesToJson(const std::vector<RunActorState>& actors) {
    Json array = Json::array();
    for (const RunActorState& actor : actors) {
        array.push_back(actorStateToJson(actor));
    }
    return array;
}

std::vector<RunActorState> actorStatesFromJson(const Json& object, const std::filesystem::path& sourcePath) {
    const Json* array = optionalField(object, "actor_states", sourcePath);
    if (array == nullptr) {
        return {};
    }

    if (!array->is_array()) {
        throwSaveError(sourcePath, "'actor_states' must be an array");
    }

    std::vector<RunActorState> result;
    result.reserve(array->size());
    for (std::size_t index = 0; index < array->size(); ++index) {
        result.push_back(actorStateFromJson(array->at(index), sourcePath));
    }
    return result;
}


std::string rewardOptionTypeToString(const RewardOptionType type) {
    switch (type) {
        case RewardOptionType::Gold:
            return "gold";
        case RewardOptionType::CardChoice:
            return "card_choice";
        case RewardOptionType::Consumable:
            return "consumable";
        case RewardOptionType::Relic:
            return "relic";
    }

    return "gold";
}

RewardOptionType rewardOptionTypeFromString(const std::string& value, const std::filesystem::path& sourcePath) {
    if (value == "gold") {
        return RewardOptionType::Gold;
    }
    if (value == "card_choice") {
        return RewardOptionType::CardChoice;
    }
    if (value == "consumable") {
        return RewardOptionType::Consumable;
    }
    if (value == "relic") {
        return RewardOptionType::Relic;
    }

    throwSaveError(sourcePath, "Unknown reward option type '" + value + "'");
}

Json rewardOptionToJson(const RewardOption& option) {
    Json cardOptions = Json::array();
    for (const CardRewardOption& cardOption : option.cardOptions) {
        cardOptions.push_back(cardOption.cardId.value);
    }

    return Json{
        {"type", rewardOptionTypeToString(option.type)},
        {"gold", option.gold},
        {"card_options", cardOptions},
        {"consumable_id", option.consumableId},
        {"relic_id", option.relicId}
    };
}

RewardOption rewardOptionFromJson(const Json& json, const std::filesystem::path& sourcePath) {
    RewardOption option;
    option.type = rewardOptionTypeFromString(requiredString(json, "type", sourcePath), sourcePath);
    option.gold = requiredInt(json, "gold", sourcePath);
    option.consumableId = requiredString(json, "consumable_id", sourcePath);
    option.relicId = requiredString(json, "relic_id", sourcePath);

    const Json cardOptions = requiredField(json, "card_options", sourcePath);
    if (!cardOptions.is_array()) {
        throwSaveError(sourcePath, "'card_options' must be an array");
    }
    for (std::size_t index = 0; index < cardOptions.size(); ++index) {
        const Json& cardId = cardOptions.at(index);
        if (!cardId.is_string()) {
            throwSaveError(sourcePath, "'card_options' element " + std::to_string(index) + " must be a string");
        }
        option.cardOptions.push_back(CardRewardOption{CardId(cardId.get<std::string>())});
    }

    return option;
}

Json rewardStateToJson(const RewardState& reward) {
    Json options = Json::array();
    for (const RewardOption& option : reward.options) {
        options.push_back(rewardOptionToJson(option));
    }

    return Json{
        {"source_node_type", nodeTypeToString(reward.sourceNodeType)},
        {"options", options}
    };
}

RewardState rewardStateFromJson(const Json& json, const std::filesystem::path& sourcePath) {
    RewardState reward;
    reward.sourceNodeType = nodeTypeFromString(requiredString(json, "source_node_type", sourcePath), sourcePath);

    const Json options = requiredField(json, "options", sourcePath);
    if (!options.is_array()) {
        throwSaveError(sourcePath, "'options' must be an array");
    }
    for (std::size_t index = 0; index < options.size(); ++index) {
        reward.options.push_back(rewardOptionFromJson(options.at(index), sourcePath));
    }

    return reward;
}


int optionalInt(const Json& object, const std::string& key, const std::filesystem::path& sourcePath, int fallback);

std::string shopStateModeToString(const ShopStateMode mode) {
    switch (mode) {
        case ShopStateMode::Shop:
            return "shop";
        case ShopStateMode::MerchantRest:
            return "merchant_rest";
    }

    return "shop";
}

ShopStateMode shopStateModeFromString(const std::string& value, const std::filesystem::path& sourcePath) {
    if (value == "shop") {
        return ShopStateMode::Shop;
    }
    if (value == "merchant_rest") {
        return ShopStateMode::MerchantRest;
    }

    throwSaveError(sourcePath, "Unknown shop state mode '" + value + "'");
}

std::string shopOfferTypeToString(const ShopOfferType type) {
    switch (type) {
        case ShopOfferType::Card:
            return "card";
        case ShopOfferType::Relic:
            return "relic";
        case ShopOfferType::Consumable:
            return "consumable";
        case ShopOfferType::CardRemoval:
            return "card_removal";
    }

    return "card";
}

ShopOfferType shopOfferTypeFromString(const std::string& value, const std::filesystem::path& sourcePath) {
    if (value == "card") {
        return ShopOfferType::Card;
    }
    if (value == "relic") {
        return ShopOfferType::Relic;
    }
    if (value == "consumable") {
        return ShopOfferType::Consumable;
    }
    if (value == "card_removal") {
        return ShopOfferType::CardRemoval;
    }

    throwSaveError(sourcePath, "Unknown shop offer type '" + value + "'");
}

Json shopOfferToJson(const ShopOffer& offer) {
    return Json{
        {"type", shopOfferTypeToString(offer.type)},
        {"content_id", offer.contentId},
        {"price", offer.price},
        {"purchased", offer.purchased}
    };
}

ShopOffer shopOfferFromJson(const Json& json, const std::filesystem::path& sourcePath) {
    ShopOffer offer;
    offer.type = shopOfferTypeFromString(requiredString(json, "type", sourcePath), sourcePath);
    offer.contentId = requiredString(json, "content_id", sourcePath);
    offer.price = requiredInt(json, "price", sourcePath);
    const Json purchased = requiredField(json, "purchased", sourcePath);
    if (!purchased.is_boolean()) {
        throwSaveError(sourcePath, "'purchased' must be a boolean");
    }
    offer.purchased = purchased.get<bool>();
    return offer;
}

Json shopStateToJson(const ShopState& shop) {
    Json offers = Json::array();
    for (const ShopOffer& offer : shop.offers) {
        offers.push_back(shopOfferToJson(offer));
    }

    return Json{
        {"mode", shopStateModeToString(shop.mode)},
        {"offers", offers},
        {"card_removal_price", shop.cardRemovalPrice},
        {"card_removal_used", shop.cardRemovalUsed},
        {"max_card_purchases", shop.maxCardPurchases},
        {"card_purchases_made", shop.cardPurchasesMade}
    };
}

ShopState shopStateFromJson(const Json& json, const std::filesystem::path& sourcePath) {
    ShopState shop;
    if (const Json* mode = optionalField(json, "mode", sourcePath)) {
        if (!mode->is_string()) {
            throwSaveError(sourcePath, "'mode' must be a string");
        }
        shop.mode = shopStateModeFromString(mode->get<std::string>(), sourcePath);
    }
    shop.cardRemovalPrice = requiredInt(json, "card_removal_price", sourcePath);
    const Json cardRemovalUsed = requiredField(json, "card_removal_used", sourcePath);
    if (!cardRemovalUsed.is_boolean()) {
        throwSaveError(sourcePath, "'card_removal_used' must be a boolean");
    }
    shop.cardRemovalUsed = cardRemovalUsed.get<bool>();
    shop.maxCardPurchases = optionalInt(json, "max_card_purchases", sourcePath, 0);
    shop.cardPurchasesMade = optionalInt(json, "card_purchases_made", sourcePath, 0);
    if (shop.maxCardPurchases < 0 || shop.cardPurchasesMade < 0) {
        throwSaveError(sourcePath, "shop card purchase counters must not be negative");
    }
    shop.cardPurchasesMade = std::min(shop.cardPurchasesMade, shop.maxCardPurchases);

    const Json offers = requiredField(json, "offers", sourcePath);
    if (!offers.is_array()) {
        throwSaveError(sourcePath, "'offers' must be an array");
    }
    for (std::size_t index = 0; index < offers.size(); ++index) {
        shop.offers.push_back(shopOfferFromJson(offers.at(index), sourcePath));
    }

    return shop;
}

std::string pendingRoomTypeToString(const RunPendingRoomType type) {
    switch (type) {
        case RunPendingRoomType::None:
            return "none";
        case RunPendingRoomType::CombatReward:
            return "combat_reward";
        case RunPendingRoomType::ChestReward:
            return "chest_reward";
        case RunPendingRoomType::Shop:
            return "shop";
        case RunPendingRoomType::MerchantRest:
            return "merchant_rest";
        case RunPendingRoomType::Event:
            return "event";
    }

    return "none";
}

RunPendingRoomType pendingRoomTypeFromString(const std::string& value, const std::filesystem::path& sourcePath) {
    if (value == "none") {
        return RunPendingRoomType::None;
    }
    if (value == "combat_reward") {
        return RunPendingRoomType::CombatReward;
    }
    if (value == "chest_reward") {
        return RunPendingRoomType::ChestReward;
    }
    if (value == "shop") {
        return RunPendingRoomType::Shop;
    }
    if (value == "merchant_rest") {
        return RunPendingRoomType::MerchantRest;
    }
    if (value == "event") {
        return RunPendingRoomType::Event;
    }

    throwSaveError(sourcePath, "Unknown pending room type '" + value + "'");
}

Json pendingRoomToJson(const RunPendingRoomState& pending) {
    return Json{
        {"type", pendingRoomTypeToString(pending.type)},
        {"node_id", pending.nodeId},
        {"reward", rewardStateToJson(pending.reward)},
        {"shop", shopStateToJson(pending.shop)},
        {"event_id", pending.eventId}
    };
}

RunPendingRoomState pendingRoomFromJson(const Json& json, const std::filesystem::path& sourcePath) {
    RunPendingRoomState pending;
    pending.type = pendingRoomTypeFromString(requiredString(json, "type", sourcePath), sourcePath);
    pending.nodeId = requiredInt(json, "node_id", sourcePath);
    pending.reward = rewardStateFromJson(requiredField(json, "reward", sourcePath), sourcePath);
    pending.shop = shopStateFromJson(requiredField(json, "shop", sourcePath), sourcePath);
    pending.eventId = requiredString(json, "event_id", sourcePath);
    return pending;
}

int optionalInt(const Json& object, const std::string& key, const std::filesystem::path& sourcePath, const int fallback) {
    const Json* value = optionalField(object, key, sourcePath);
    if (value == nullptr) {
        return fallback;
    }

    if (!value->is_number_integer()) {
        throwSaveError(sourcePath, "'" + key + "' must be an integer");
    }

    return value->get<int>();
}

Json statsToJson(const RunStats& stats) {
    return Json{
        {"combats_won", stats.combatsWon},
        {"combats_lost", stats.combatsLost},
        {"elites_killed", stats.elitesKilled},
        {"bosses_killed", stats.bossesKilled},
        {"enemies_killed", stats.enemiesKilled},
        {"events_completed", stats.eventsCompleted},
        {"shops_visited", stats.shopsVisited},
        {"chests_opened", stats.chestsOpened},
        {"rests_used", stats.restsUsed},
        {"rest_heals_used", stats.restHealsUsed},
        {"rest_upgrades_used", stats.restUpgradesUsed},
        {"rest_skips", stats.restSkips},
        {"damage_taken", stats.damageTaken},
        {"consumables_used", stats.consumablesUsed},
        {"gold_gained", stats.goldGained},
        {"gold_spent", stats.goldSpent},
        {"cards_added", stats.cardsAdded},
        {"cards_removed", stats.cardsRemoved},
        {"cards_upgraded", stats.cardsUpgraded},
        {"cards_skipped", stats.cardsSkipped},
        {"rewards_skipped", stats.rewardsSkipped},
        {"relics_gained", stats.relicsGained},
        {"consumables_gained", stats.consumablesGained},
        {"nodes_completed", stats.nodesCompleted}
    };
}

RunStats statsFromJson(const Json& json, const std::filesystem::path& sourcePath) {
    RunStats stats;
    stats.combatsWon = requiredInt(json, "combats_won", sourcePath);
    stats.combatsLost = optionalInt(json, "combats_lost", sourcePath, 0);
    stats.elitesKilled = requiredInt(json, "elites_killed", sourcePath);
    stats.bossesKilled = requiredInt(json, "bosses_killed", sourcePath);
    stats.enemiesKilled = optionalInt(json, "enemies_killed", sourcePath, stats.elitesKilled + stats.bossesKilled);
    stats.eventsCompleted = optionalInt(json, "events_completed", sourcePath, 0);
    stats.shopsVisited = optionalInt(json, "shops_visited", sourcePath, 0);
    stats.chestsOpened = optionalInt(json, "chests_opened", sourcePath, 0);
    stats.restsUsed = optionalInt(json, "rests_used", sourcePath, 0);
    stats.restHealsUsed = optionalInt(json, "rest_heals_used", sourcePath, 0);
    stats.restUpgradesUsed = optionalInt(json, "rest_upgrades_used", sourcePath, 0);
    stats.restSkips = optionalInt(json, "rest_skips", sourcePath, 0);
    stats.damageTaken = optionalInt(json, "damage_taken", sourcePath, 0);
    stats.consumablesUsed = optionalInt(json, "consumables_used", sourcePath, 0);
    stats.goldGained = requiredInt(json, "gold_gained", sourcePath);
    stats.goldSpent = optionalInt(json, "gold_spent", sourcePath, 0);
    stats.cardsAdded = requiredInt(json, "cards_added", sourcePath);
    stats.cardsRemoved = optionalInt(json, "cards_removed", sourcePath, 0);
    stats.cardsUpgraded = optionalInt(json, "cards_upgraded", sourcePath, 0);
    stats.cardsSkipped = optionalInt(json, "cards_skipped", sourcePath, 0);
    stats.rewardsSkipped = optionalInt(json, "rewards_skipped", sourcePath, 0);
    stats.relicsGained = optionalInt(json, "relics_gained", sourcePath, 0);
    stats.consumablesGained = optionalInt(json, "consumables_gained", sourcePath, 0);
    stats.nodesCompleted = requiredInt(json, "nodes_completed", sourcePath);
    return stats;
}
}

Json RunStateSerializer::toJson(const RunState& run) {
    return Json{
        {"version", 1},
        {"archetype_id", run.archetypeId.value},
        {"difficulty_id", run.difficultyId.value},
        {"archetype_mechanic_id", run.archetypeMechanicId},
        {"seed", run.seed},
        {"random_state", run.randomState},
        {"gold", run.gold},
        {"act", run.act},
        {"act_completed", run.actCompleted},
        {"completed_act", run.completedAct},
        {"defeated_boss_enemy_ids", stringArray(run.defeatedBossEnemyIds)},
        {"enemy_hp_multiplier", run.enemyHpMultiplier},
        {"enemy_damage_multiplier", run.enemyDamageMultiplier},
        {"gold_reward_multiplier", run.goldRewardMultiplier},
        {"deck_card_ids", cardIdArray(run.deckCardIds)},
        {"upgraded_deck_indices", intArray(run.upgradedDeckIndices)},
        {"relic_ids", stringArray(run.relicIds)},
        {"consumable_ids", stringArray(run.consumableIds)},
        {"max_consumables", run.maxConsumables},
        {"actor_definition_ids", stringArray(run.actorDefinitionIds)},
        {"reward_card_pool_ids", stringArray(run.rewardCardPoolIds)},
        {"actor_states", actorStatesToJson(run.actorStates)},
        {"map", mapToJson(run.map)},
        {"stats", statsToJson(run.stats)},
        {"pending_room", pendingRoomToJson(run.pendingRoom)}
    };
}

RunState RunStateSerializer::fromJson(const Json& json, const std::filesystem::path& sourcePath) {
    const int version = requiredInt(json, "version", sourcePath);
    if (version != 1) {
        throwSaveError(sourcePath, "Unsupported run save version " + std::to_string(version));
    }

    RunState run;
    run.archetypeId = PlayableArchetypeId(requiredString(json, "archetype_id", sourcePath));
    run.difficultyId = DifficultyId(requiredString(json, "difficulty_id", sourcePath));
    run.archetypeMechanicId = requiredString(json, "archetype_mechanic_id", sourcePath);
    run.seed = requiredUnsigned(json, "seed", sourcePath);
    if (const Json* randomState = optionalField(json, "random_state", sourcePath)) {
        if (!randomState->is_string()) {
            throwSaveError(sourcePath, "'random_state' must be a string");
        }
        run.randomState = randomState->get<std::string>();
        try {
            Random validationRandom(run.seed);
            validationRandom.setState(run.randomState);
        } catch (const std::exception& error) {
            throwSaveError(sourcePath, std::string("Invalid 'random_state': ") + error.what());
        }
    }
    run.gold = requiredInt(json, "gold", sourcePath);
    run.act = requiredInt(json, "act", sourcePath);
    if (const Json* actCompleted = optionalField(json, "act_completed", sourcePath)) {
        if (!actCompleted->is_boolean()) {
            throwSaveError(sourcePath, "'act_completed' must be a boolean");
        }
        run.actCompleted = actCompleted->get<bool>();
    }
    if (const Json* completedAct = optionalField(json, "completed_act", sourcePath)) {
        if (!completedAct->is_number_integer()) {
            throwSaveError(sourcePath, "'completed_act' must be an integer");
        }
        run.completedAct = completedAct->get<int>();
    }
    if (optionalField(json, "defeated_boss_enemy_ids", sourcePath) != nullptr) {
        run.defeatedBossEnemyIds = requiredStringArray(json, "defeated_boss_enemy_ids", sourcePath);
    }
    run.enemyHpMultiplier = requiredFloat(json, "enemy_hp_multiplier", sourcePath);
    run.enemyDamageMultiplier = requiredFloat(json, "enemy_damage_multiplier", sourcePath);
    run.goldRewardMultiplier = requiredFloat(json, "gold_reward_multiplier", sourcePath);
    run.deckCardIds = requiredCardIdArray(json, "deck_card_ids", sourcePath);

    if (const Json* upgradedDeckIndices = optionalField(json, "upgraded_deck_indices", sourcePath)) {
        run.upgradedDeckIndices = intArrayFromJson(*upgradedDeckIndices, "upgraded_deck_indices", sourcePath);
    } else if (optionalField(json, "upgraded_card_ids", sourcePath) != nullptr) {
        run.upgradedDeckIndices = migrateLegacyUpgradedCardIds(
            run.deckCardIds,
            requiredCardIdArray(json, "upgraded_card_ids", sourcePath)
        );
    }

    normalizeUpgradedDeckIndices(run);
    run.relicIds = requiredStringArray(json, "relic_ids", sourcePath);
    run.consumableIds = requiredStringArray(json, "consumable_ids", sourcePath);
    run.maxConsumables = requiredInt(json, "max_consumables", sourcePath);
    run.actorDefinitionIds = requiredStringArray(json, "actor_definition_ids", sourcePath);
    if (optionalField(json, "reward_card_pool_ids", sourcePath) != nullptr) {
        run.rewardCardPoolIds = requiredStringArray(json, "reward_card_pool_ids", sourcePath);
    } else {
        run.rewardCardPoolIds = run.actorDefinitionIds;
    }
    run.actorStates = actorStatesFromJson(json, sourcePath);
    run.map = mapFromJson(requiredField(json, "map", sourcePath), sourcePath);
    run.stats = statsFromJson(requiredField(json, "stats", sourcePath), sourcePath);

    if (const Json* pendingRoom = optionalField(json, "pending_room", sourcePath)) {
        run.pendingRoom = pendingRoomFromJson(*pendingRoom, sourcePath);
    }

    return run;
}
