#pragma once

#include <string>
#include <utility>

struct ActiveItemId {
    std::string value;

    ActiveItemId() = default;
    explicit ActiveItemId(std::string id) : value(std::move(id)) {}

    bool operator==(const ActiveItemId&) const = default;
};
