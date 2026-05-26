#pragma once

#include <string>
#include <utility>

struct ConsumableId {
    std::string value;

    ConsumableId() = default;
    explicit ConsumableId(std::string id) : value(std::move(id)) {}

    bool operator==(const ConsumableId& other) const { return value == other.value; }
    bool operator!=(const ConsumableId& other) const { return !(*this == other); }
};
