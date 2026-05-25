#pragma once

#include <string>
#include <string_view>

enum class StatusType {
    Buff,
    Debuff,
    Neutral
};

std::string toString(StatusType type);

StatusType statusTypeFromString(std::string_view value);
