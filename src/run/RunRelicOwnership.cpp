#include "run/RunRelicOwnership.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace RunRelicOwnership {
namespace {
void appendUnique(std::vector<std::string>& values, const std::string& value) {
    if (value.empty()) {
        return;
    }

    if (std::find(values.begin(), values.end(), value) == values.end()) {
        values.push_back(value);
    }
}

RunActorState* findActor(RunState& run, const std::string& actorDefinitionId) {
    for (RunActorState& actor : run.actorStates) {
        if (actor.definitionId == actorDefinitionId) {
            return &actor;
        }
    }

    return nullptr;
}
}

std::vector<std::string> aggregateRelicIds(const RunState& run) {
    std::vector<std::string> result;

    for (const RunActorState& actor : run.actorStates) {
        for (const std::string& relicId : actor.relicIds) {
            appendUnique(result, relicId);
        }
    }

    for (const std::string& relicId : run.relicIds) {
        appendUnique(result, relicId);
    }

    return result;
}

void rebuildLegacyRelicList(RunState& run) {
    std::vector<std::string> result;

    for (const RunActorState& actor : run.actorStates) {
        for (const std::string& relicId : actor.relicIds) {
            appendUnique(result, relicId);
        }
    }

    run.relicIds = std::move(result);
}

std::string defaultActorDefinitionId(const RunState& run) {
    for (const RunActorState& actor : run.actorStates) {
        if (!actor.definitionId.empty()) {
            return actor.definitionId;
        }
    }

    if (!run.actorDefinitionIds.empty()) {
        return run.actorDefinitionIds.front();
    }

    return {};
}

bool hasActor(const RunState& run, const std::string& actorDefinitionId) {
    if (actorDefinitionId.empty()) {
        return false;
    }

    return std::any_of(run.actorStates.begin(), run.actorStates.end(), [&actorDefinitionId](const RunActorState& actor) {
        return actor.definitionId == actorDefinitionId;
    }) || std::find(run.actorDefinitionIds.begin(), run.actorDefinitionIds.end(), actorDefinitionId) != run.actorDefinitionIds.end();
}

bool actorOwnsRelic(const RunState& run, const std::string& actorDefinitionId, const std::string& relicId) {
    for (const RunActorState& actor : run.actorStates) {
        if (actor.definitionId != actorDefinitionId) {
            continue;
        }

        return std::find(actor.relicIds.begin(), actor.relicIds.end(), relicId) != actor.relicIds.end();
    }

    return false;
}

bool ownsRelic(const RunState& run, const std::string& relicId) {
    for (const RunActorState& actor : run.actorStates) {
        if (std::find(actor.relicIds.begin(), actor.relicIds.end(), relicId) != actor.relicIds.end()) {
            return true;
        }
    }

    return std::find(run.relicIds.begin(), run.relicIds.end(), relicId) != run.relicIds.end();
}

bool assignRelicToActor(RunState& run, const std::string& relicId, const std::string& actorDefinitionId) {
    if (relicId.empty() || ownsRelic(run, relicId)) {
        return false;
    }

    std::string ownerActorDefinitionId = actorDefinitionId;
    if (ownerActorDefinitionId.empty() || !hasActor(run, ownerActorDefinitionId)) {
        ownerActorDefinitionId = defaultActorDefinitionId(run);
    }

    if (ownerActorDefinitionId.empty()) {
        return false;
    }

    RunActorState* actor = findActor(run, ownerActorDefinitionId);
    if (actor == nullptr) {
        RunActorState created;
        created.definitionId = ownerActorDefinitionId;
        run.actorStates.push_back(std::move(created));
        actor = &run.actorStates.back();
    }

    actor->relicIds.push_back(relicId);
    rebuildLegacyRelicList(run);
    return true;
}

void migrateLegacyRelicsToActors(RunState& run) {
    const bool alreadyHasActorRelics = std::any_of(run.actorStates.begin(), run.actorStates.end(), [](const RunActorState& actor) {
        return !actor.relicIds.empty();
    });

    if (alreadyHasActorRelics || run.relicIds.empty()) {
        rebuildLegacyRelicList(run);
        return;
    }

    if (run.actorStates.empty()) {
        return;
    }

    for (std::size_t index = 0; index < run.relicIds.size(); ++index) {
        RunActorState& actor = run.actorStates[index % run.actorStates.size()];
        appendUnique(actor.relicIds, run.relicIds[index]);
    }

    rebuildLegacyRelicList(run);
}
} // namespace RunRelicOwnership
