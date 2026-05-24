#pragma once

#include <string>

struct TextId {
    std::string id;

    TextId() = default;

    explicit TextId(std::string id) : id(std::move(id)) {}
};