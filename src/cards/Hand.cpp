#include "Hand.hpp"

#include <algorithm>
#include <stdexcept>

bool Hand::empty() const {
    return cards_.empty();
}

std::size_t Hand::size() const {
    return cards_.size();
}

void Hand::clear() {
    cards_.clear();
}

void Hand::add(CardInstance card) {
    cards_.push_back(std::move(card));
}

bool Hand::contains(const CardInstanceId id) const {
    return std::any_of(cards_.begin(), cards_.end(), [id](const CardInstance& card) {
        return card.instanceId == id;
    });
}

const CardInstance& Hand::get(const CardInstanceId id) const {
    const auto iterator = std::find_if(cards_.begin(), cards_.end(), [id](const CardInstance& card) {
        return card.instanceId == id;
    });

    if (iterator == cards_.end()) {
        throw std::runtime_error("Unknown card instance in hand");
    }

    return *iterator;
}

CardInstance& Hand::get(const CardInstanceId id) {
    const auto iterator = std::find_if(cards_.begin(), cards_.end(), [id](const CardInstance& card) {
        return card.instanceId == id;
    });

    if (iterator == cards_.end()) {
        throw std::runtime_error("Unknown card instance in hand");
    }

    return *iterator;
}

std::optional<CardInstance> Hand::remove(const CardInstanceId id) {
    const auto iterator = std::find_if(cards_.begin(), cards_.end(), [id](const CardInstance& card) {
        return card.instanceId == id;
    });

    if (iterator == cards_.end()) {
        return std::nullopt;
    }

    CardInstance result = std::move(*iterator);
    cards_.erase(iterator);
    return result;
}

const std::vector<CardInstance>& Hand::cards() const {
    return cards_;
}

std::vector<CardInstance>& Hand::cards() {
    return cards_;
}
