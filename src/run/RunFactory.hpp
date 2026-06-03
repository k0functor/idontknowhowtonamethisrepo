#pragma once

#include "actors/PlayerActorDatabase.hpp"
#include "archetypes/PlayableArchetypeDefinition.hpp"
#include "run/DifficultyDefinition.hpp"
#include "run/FloorDefinition.hpp"
#include "run/RunState.hpp"
#include "run/RunMapGenerationConfig.hpp"

#include <cstdint>

class RunFactory {
public:
    RunState createRun(
        const PlayableArchetypeDefinition& archetype,
        const DifficultyDefinition& difficulty,
        const PlayerActorDatabase& actors,
        const RunMapGenerationConfig& mapGeneration,
        const FloorDefinition& floor,
        std::uint32_t seed
    ) const;
};
