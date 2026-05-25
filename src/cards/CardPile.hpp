#pragma once

#include "cards/CardInstance.hpp"
#include "core/Random.hpp"

#include <cstddef>
#include <optional>
#include <vector>

class CardPile {
public:
    bool empty() const;
    std::size_t size() const;

    void clear();

    void addTop(CardInstance card);
    void addBottom(CardInstance card);

    CardInstance drawTop();

    bool contains(CardInstanceId id) const;
    std::optional<CardInstance> remove(CardInstanceId id);

    void shuffle(Random& random);

    const std::vector<CardInstance>& cards() const;
    std::vector<CardInstance>& cards();

private:
    std::vector<CardInstance> cards_;
};
