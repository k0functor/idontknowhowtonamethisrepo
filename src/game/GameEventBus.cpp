#include "GameEventBus.hpp"

#include <utility>

void GameEventBus::clear() {
    listeners_.clear();
}

void GameEventBus::subscribe(Listener listener) {
    listeners_.push_back(std::move(listener));
}

void GameEventBus::emit(const GameEvent& event) const {
    for (const Listener& listener : listeners_) {
        listener(event);
    }
}
