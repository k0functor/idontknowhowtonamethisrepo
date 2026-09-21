#include "events/RunEventSelector.hpp"

#include "core/Random.hpp"
#include "events/RunEventRequirement.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace {
std::string seenEventFlag(const std::string& eventId) {
    return "event.seen." + eventId;
}

bool eventWasSeen(const RunState& run, const RunEventDefinition& event) {
    const std::string flag = seenEventFlag(event.id);
    return std::find(run.eventFlags.begin(), run.eventFlags.end(), flag) != run.eventFlags.end();
}
}

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

    std::vector<const RunEventDefinition*> unseen;
    unseen.reserve(available.size());
    for (const RunEventDefinition* event : available) {
        if (event != nullptr && !eventWasSeen(run, *event)) {
            unseen.push_back(event);
        }
    }

    const std::vector<const RunEventDefinition*>& pool = unseen.empty() ? available : unseen;
    const int index = random.rangeInclusive(0, static_cast<int>(pool.size()) - 1);
    return *pool[static_cast<std::size_t>(index)];
}
