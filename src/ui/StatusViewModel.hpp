#pragma once

#include <string>

struct StatusViewModel {
    std::string id;
    std::string name;
    int amount = 0;
    bool debuff = false;
};
