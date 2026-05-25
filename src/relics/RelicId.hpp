#pragma once

#include <string>

struct RelicId {
    std::string value;

    RelicId() = default;

    explicit RelicId(std::string id)
        : value(std::move(id)) {}

    bool operator==(const RelicId& other) const {
        return value == other.value;
    }

    bool operator!=(const RelicId& other) const {
        return !(*this == other);
    }
};
