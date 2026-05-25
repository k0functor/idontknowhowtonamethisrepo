#pragma once

#include "archetypes/PlayableArchetypeDefinition.hpp"
#include "run/DifficultyDefinition.hpp"
#include "run/RunState.hpp"

#include <cstdint>

class RunFactory {
public:
    RunState createRun(
        const PlayableArchetypeDefinition& archetype,
        const DifficultyDefinition& difficulty,
        std::uint32_t seed
    ) const;
};
