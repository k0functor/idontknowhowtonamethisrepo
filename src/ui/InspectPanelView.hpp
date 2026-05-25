#pragma once

#include "inspect/InspectPanelModel.hpp"
#include "ui/UiFont.hpp"

#include <raylib.h>

class InspectPanelView {
public:
    void render(
        const UiFont& font,
        const InspectPanelModel& model,
        Rectangle bounds
    ) const;
};
