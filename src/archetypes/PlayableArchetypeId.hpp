#pragma once

#include <string>
#include <utility>

struct PlayableArchetypeId {
    std::string value;

    PlayableArchetypeId() = default;

    explicit PlayableArchetypeId(std::string id)
        : value(std::move(id)) {}

    bool operator==(const PlayableArchetypeId& other) const {
        return value == other.value;
    }
};
