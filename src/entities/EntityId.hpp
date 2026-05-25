#pragma once

#include <cstdint>

struct EntityId {
    std::uint64_t value = 0;

    bool operator==(const EntityId& other) const {
        return value == other.value;
    }

    bool operator!=(const EntityId& other) const {
        return !(*this == other);
    }
};

class EntityIdGenerator {
public:
    EntityId create();
    void reset(std::uint64_t nextId = 1);

private:
    std::uint64_t nextId_ = 1;
};
