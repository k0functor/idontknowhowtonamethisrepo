#pragma once

#include <string>
#include <string_view>

enum class DiceCorruptionType {
    None,
    Cursed,
    Fire,
    Poison,
    Blood,
    Unstable
};

struct DiceCorruption {
    DiceCorruptionType type = DiceCorruptionType::None;
};

std::string toString(const DiceCorruption& corruption);

DiceCorruptionType parseDiceCorruptionType(std::string_view value);