#pragma once

#include <string>

struct ActiveItemState {
    std::string itemId;
    int charge = 0;

    bool empty() const { return itemId.empty(); }
    void clear() {
        itemId.clear();
        charge = 0;
    }
};
