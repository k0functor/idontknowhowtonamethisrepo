#pragma once

#include <string>

struct TextId {
    std::string value;

    TextId() = default;

    explicit TextId(std::string value) : value(std::move(value)) {}
};