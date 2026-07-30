#pragma once

#include "cards/CardInstance.hpp"

#include <cstddef>
#include <optional>
#include <vector>

class Hand {
public:
    static constexpr std::size_t MaximumSize = 10;

    bool empty() const;
    bool full() const;
    std::size_t size() const;
    std::size_t remainingCapacity() const;

    void clear();
    bool tryAdd(CardInstance card);
    void add(CardInstance card);

    bool contains(CardInstanceId id) const;

    const CardInstance& get(CardInstanceId id) const;
    CardInstance& get(CardInstanceId id);

    std::optional<CardInstance> remove(CardInstanceId id);

    const std::vector<CardInstance>& cards() const;
    std::vector<CardInstance>& cards();

private:
    std::vector<CardInstance> cards_;
};
