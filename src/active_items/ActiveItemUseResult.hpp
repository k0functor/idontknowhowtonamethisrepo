#pragma once

#include <string>

enum class ActiveItemUseStatus {
    Used,
    NoItem,
    UnknownItem,
    NotEnoughCharge,
    InvalidContext,
    NoEffect
};

struct ActiveItemUseResult {
    ActiveItemUseStatus status = ActiveItemUseStatus::NoItem;
    std::string itemId;
    int chargeSpent = 0;
    int healed = 0;
    int goldGained = 0;

    bool used() const { return status == ActiveItemUseStatus::Used; }
};
