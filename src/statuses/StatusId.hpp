#pragma once

#include <string>

struct StatusId {
    std::string value;

    StatusId() = default;

    explicit StatusId(std::string id)
        : value(std::move(id)) {}

    bool operator==(const StatusId& other) const {
        return value == other.value;
    }

    bool operator!=(const StatusId& other) const {
        return !(*this == other);
    }
};
