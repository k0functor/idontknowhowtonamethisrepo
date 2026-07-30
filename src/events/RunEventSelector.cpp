#include "events/RunEventSelector.hpp"

#include "core/Random.hpp"
#include "events/RunEventRequirement.hpp"

#include <stdexcept>

std::vector<const RunEventDefinition*> availableRunEvents(
    const std::vector<const RunEventDefinition*>& events,
    const RunState& run
) {
    std::vector<const RunEventDefinition*> available;
    available.reserve(events.size());
    for (const RunEventDefinition* event : events) {
        if (event != nullptr && evaluateRunEventChoiceRequirements(event->requirements, run).available) {
            available.push_back(event);
        }
    }
    return available;
}

const RunEventDefinition& chooseAvailableRunEvent(
    const std::vector<const RunEventDefinition*>& events,
    const RunState& run,
    Random& random
) {
    const std::vector<const RunEventDefinition*> available = availableRunEvents(events, run);
    if (available.empty()) {
        throw std::runtime_error("Cannot choose run event: no event requirements are satisfied");
    }
    const int index = random.rangeInclusive(0, static_cast<int>(available.size()) - 1);
    return *available[static_cast<std::size_t>(index)];
}
