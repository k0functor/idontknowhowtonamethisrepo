#pragma once

#include "run/RunState.hpp"

#include <string>
#include <vector>

namespace RunRelicOwnership {
std::vector<std::string> aggregateRelicIds(const RunState& run);
void rebuildLegacyRelicList(RunState& run);
void migrateLegacyRelicsToActors(RunState& run);

std::string defaultActorDefinitionId(const RunState& run);
bool hasActor(const RunState& run, const std::string& actorDefinitionId);
bool ownsRelic(const RunState& run, const std::string& relicId);
bool actorOwnsRelic(const RunState& run, const std::string& actorDefinitionId, const std::string& relicId);
bool assignRelicToActor(RunState& run, const std::string& relicId, const std::string& actorDefinitionId);
} // namespace RunRelicOwnership
