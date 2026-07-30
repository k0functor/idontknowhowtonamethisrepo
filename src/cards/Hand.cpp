#include "Hand.hpp"

#include <algorithm>
#include <stdexcept>

bool Hand::empty() const {
    return cards_.empty();
}

bool Hand::full() const {
    return cards_.size() >= MaximumSize;
}

std::size_t Hand::size() const {
    return cards_.size();
}

std::size_t Hand::remainingCapacity() const {
    return cards_.size() >= MaximumSize ? 0u : MaximumSize - cards_.size();
}

void Hand::clear() {
    cards_.clear();
}

bool Hand::tryAdd(CardInstance card) {
    if (full()) {
        return false;
    }
    cards_.push_back(std::move(card));
    return true;
}

void Hand::add(CardInstance card) {
    if (!tryAdd(std::move(card))) {
        throw std::runtime_error("Cannot add card to a full hand");
    }
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
