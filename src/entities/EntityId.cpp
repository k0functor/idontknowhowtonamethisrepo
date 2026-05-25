#include "EntityId.hpp"

EntityId EntityIdGenerator::create() {
    return EntityId{nextId_++};
}

void EntityIdGenerator::reset(const std::uint64_t nextId) {
    nextId_ = nextId;
}
