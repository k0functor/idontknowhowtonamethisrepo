#pragma once

#include <string>

struct ConsumableViewModel {
    std::string id;
    std::string name;
    std::string description;
    bool filled = false;
};
