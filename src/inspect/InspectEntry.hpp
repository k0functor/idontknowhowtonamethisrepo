#pragma once

#include <string>

enum class InspectEntryStyle {
    Normal,
    Hint,
    Warning
};

struct InspectEntry {
    std::string title;
    std::string description;
    InspectEntryStyle style = InspectEntryStyle::Normal;
};
