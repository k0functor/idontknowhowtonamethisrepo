#pragma once

#include <string>
#include <utility>

struct CardId {
    std::string value;

    CardId() = default;

    explicit CardId(std::string id)
        : value(std::move(id)) {}

    bool operator==(const CardId& other) const {
        return value == other.value;
    }

    bool operator!=(const CardId& other) const {
        return !(*this == other);
    }
};
