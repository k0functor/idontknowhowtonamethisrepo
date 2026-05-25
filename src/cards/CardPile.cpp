#include "CardPile.hpp"

#include <algorithm>
#include <stdexcept>

bool CardPile::empty() const {
    return cards_.empty();
}

std::size_t CardPile::size() const {
    return cards_.size();
}

void CardPile::clear() {
    cards_.clear();
}

void CardPile::addTop(CardInstance card) {
    cards_.push_back(std::move(card));
}

void CardPile::addBottom(CardInstance card) {
    cards_.insert(cards_.begin(), std::move(card));
}

CardInstance CardPile::drawTop() {
    if (cards_.empty()) {
        throw std::runtime_error("Cannot draw from an empty card pile");
    }

    CardInstance card = std::move(cards_.back());
    cards_.pop_back();
    return card;
}

bool CardPile::contains(const CardInstanceId id) const {
    return std::any_of(cards_.begin(), cards_.end(), [id](const CardInstance& card) {
        return card.instanceId == id;
    });
}

std::optional<CardInstance> CardPile::remove(const CardInstanceId id) {
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

void CardPile::shuffle(Random& random) {
    if (cards_.size() < 2) {
        return;
    }

    for (std::size_t i = cards_.size() - 1; i > 0; --i) {
        const int j = random.rangeInclusive(0, static_cast<int>(i));
        std::swap(cards_[i], cards_[static_cast<std::size_t>(j)]);
    }
}

const std::vector<CardInstance>& CardPile::cards() const {
    return cards_;
}

std::vector<CardInstance>& CardPile::cards() {
    return cards_;
}
