#pragma once

#include "events/RunEventEffect.hpp"
#include "localization/TextId.hpp"

#include <string>
#include <vector>

struct RunEventChoiceDefinition {
    TextId textTextId;
    TextId descriptionTextId;
    std::vector<RunEventEffect> effects;
};

struct RunEventDefinition {
    std::string id;
    TextId titleTextId;
    TextId descriptionTextId;
    std::vector<RunEventChoiceDefinition> choices;
};
