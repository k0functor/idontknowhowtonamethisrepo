#pragma once

#include "inspect/InspectEntry.hpp"

#include <string>
#include <vector>

struct InspectPanelModel {
    std::string header;
    std::string subheader;
    std::vector<InspectEntry> entries;

    bool empty() const {
        return header.empty() && subheader.empty() && entries.empty();
    }
};
