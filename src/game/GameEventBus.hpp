#pragma once

#include "game/GameEvent.hpp"

#include <functional>
#include <vector>

class GameEventBus {
public:
    using Listener = std::function<void(const GameEvent&)>;

    void clear();
    void subscribe(Listener listener);
    void emit(const GameEvent& event) const;

private:
    std::vector<Listener> listeners_;
};
