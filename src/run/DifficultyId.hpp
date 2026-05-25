#pragma once

#include <string>
#include <utility>

struct DifficultyId {
    std::string value;

    DifficultyId() = default;

    explicit DifficultyId(std::string id)
        : value(std::move(id)) {}

    bool operator==(const DifficultyId& other) const {
        return value == other.value;
    }
};
