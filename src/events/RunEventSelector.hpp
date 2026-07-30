#pragma once

#include "events/RunEventDefinition.hpp"
#include "run/RunState.hpp"

#include <vector>

class Random;

std::vector<const RunEventDefinition*> availableRunEvents(
    const std::vector<const RunEventDefinition*>& events,
    const RunState& run
);

const RunEventDefinition& chooseAvailableRunEvent(
    const std::vector<const RunEventDefinition*>& events,
    const RunState& run,
    Random& random
);
