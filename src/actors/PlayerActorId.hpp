#pragma once

#include <string>
#include <utility>

struct PlayerActorId {
    std::string value;

    PlayerActorId() = default;

    explicit PlayerActorId(std::string id)
        : value(std::move(id)) {}

    bool operator==(const PlayerActorId& other) const {
        return value == other.value;
    }
};
