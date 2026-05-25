#pragma once

#include "cards/CardId.hpp"
#include "cards/CardInstance.hpp"

#include <cstdint>

class CardInstanceFactory {
public:
    CardInstance create(const CardId& definitionId);

    void reset(std::uint64_t nextId = 1);

private:
    std::uint64_t nextId_ = 1;
};
