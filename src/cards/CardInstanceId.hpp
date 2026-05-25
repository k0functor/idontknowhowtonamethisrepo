#pragma once

#include <cstdint>

struct CardInstanceId {
    std::uint64_t value = 0;

    bool operator==(const CardInstanceId& other) const {
        return value == other.value;
    }

    bool operator!=(const CardInstanceId& other) const {
        return !(*this == other);
    }
};
