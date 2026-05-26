#pragma once

#include <string>
#include <utility>

struct DroneId {
    std::string value;

    DroneId() = default;

    explicit DroneId(std::string id)
        : value(std::move(id)) {}

    bool operator==(const DroneId& other) const {
        return value == other.value;
    }

    bool operator!=(const DroneId& other) const {
        return !(*this == other);
    }
};
