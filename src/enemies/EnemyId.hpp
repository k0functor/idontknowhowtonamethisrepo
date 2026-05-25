#pragma once

#include <string>
#include <utility>

struct EnemyId {
    std::string value;

    EnemyId() = default;

    explicit EnemyId(std::string id)
        : value(std::move(id)) {}
};
