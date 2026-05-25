#pragma once

#include "cards/CardInstance.hpp"

#include <cstddef>
#include <optional>
#include <vector>

class Hand {
public:
    bool empty() const;
    std::size_t size() const;

    void clear();
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
