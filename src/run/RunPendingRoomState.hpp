#pragma once

#include "rewards/RewardState.hpp"
#include "shop/ShopState.hpp"

#include <string>

// Persistent state for a non-map room that has already been generated but not finished yet.
// This prevents rerolling shops/events/chests/reward screens by saving and loading mid-room.
enum class RunPendingRoomType {
    None,
    CombatReward,
    ChestReward,
    Shop,
    MerchantRest,
    Event
};

struct RunPendingRoomState {
    RunPendingRoomType type = RunPendingRoomType::None;
    int nodeId = -1;

    RewardState reward;
    ShopState shop;
    std::string eventId;

    bool active() const {
        return type != RunPendingRoomType::None;
    }

    void clear() {
        type = RunPendingRoomType::None;
        nodeId = -1;
        reward = RewardState{};
        shop = ShopState{};
        eventId.clear();
    }
};
