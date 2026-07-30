#pragma once

#include <string>

enum class RunMapNodeType {
    Combat,
    Elite,
    Event,
    Shop,
    Chest,
    Rest,
    Boss
};

std::string toString(RunMapNodeType type);
