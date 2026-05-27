#include "CardInstanceFactory.hpp"

CardInstance CardInstanceFactory::create(const CardId& definitionId, const bool upgraded) {
    CardInstance instance;
    instance.instanceId = CardInstanceId{nextId_++};
    instance.definitionId = definitionId;
    instance.upgraded = upgraded;
    return instance;
}

void CardInstanceFactory::reset(const std::uint64_t nextId) {
    nextId_ = nextId;
}
