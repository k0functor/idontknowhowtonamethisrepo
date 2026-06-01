#pragma once

#include <string>

struct StatusViewModel {
    std::string id;
    std::string name;
    std::string description;
    std::string typeLabel;
    std::string durationLabel;
    std::string runtimeText;

    int amount = 0;

    bool buff = false;
    bool debuff = false;
};
