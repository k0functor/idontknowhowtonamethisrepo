#pragma once

#include "active_items/ActiveItemDatabase.hpp"
#include "localization/LocalizationManager.hpp"
#include "run/RunState.hpp"
#include "ui/UiFont.hpp"

#include <string>

#include <raylib.h>

class ActiveItemComparisonView {
public:
    static void render(
        const UiFont& font,
        const LocalizationManager& localization,
        const ActiveItemDatabase& activeItems,
        const RunState& run,
        const std::string& offeredItemId,
        Rectangle bounds
    );
};
