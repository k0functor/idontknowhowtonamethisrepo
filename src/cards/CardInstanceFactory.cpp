#include "CardInstanceFactory.hpp"

CardInstance CardInstanceFactory::create(const CardId& definitionId) {
    CardInstance instance;
    instance.instanceId = CardInstanceId{nextId_++};
    instance.definitionId = definitionId;
    return instance;
}

void CardInstanceFactory::reset(const std::uint64_t nextId) {
    nextId_ = nextId;
}
