#include "RunFactory.hpp"

#include "core/Random.hpp"
#include "run/RunMapGenerator.hpp"
#include "run/StressRules.hpp"

#include <utility>

RunState RunFactory::createRun(
    const PlayableArchetypeDefinition& archetype,
    const DifficultyDefinition& difficulty,
    const PlayerActorDatabase& actors,
    const RunMapGenerationConfig& mapGeneration,
    const std::uint32_t seed
) const {
    RunMapGenerator generator;
    Random mapRandom(seed);

    RunState run;
    run.archetypeId = archetype.id;
    run.difficultyId = difficulty.id;
    run.archetypeMechanicId = archetype.mechanicId;
    run.seed = seed;
    run.gold = archetype.startingGold;
    run.act = 1;
    run.enemyHpMultiplier = difficulty.enemyHpMultiplier;
    run.enemyDamageMultiplier = difficulty.enemyDamageMultiplier;
    run.goldRewardMultiplier = difficulty.goldMultiplier;
    run.actorDefinitionIds = archetype.actorDefinitionIds;
    run.actorStates.reserve(archetype.actorDefinitionIds.size());
    for (const std::string& actorId : archetype.actorDefinitionIds) {
        const PlayerActorDefinition& actor = actors.get(PlayerActorId(actorId));
        RunActorState actorState;
        actorState.definitionId = actorId;
        actorState.currentHp = actor.maxHp;
        actorState.maxHp = actor.maxHp;
        actorState.stress = 0;
        actorState.maxStress = StressRules::MaximumStress;
        actorState.resolveCheckTriggered = false;
        actorState.traitIds = actor.startingTraitIds;
        run.actorStates.push_back(std::move(actorState));
    }
    run.relicIds = archetype.startingRelicIds;

    run.deckCardIds.reserve(archetype.startingDeckCardIds.size());
    for (const std::string& cardId : archetype.startingDeckCardIds) {
        run.deckCardIds.emplace_back(cardId);
    }

    run.map = generator.generateActOneMap(mapRandom, mapGeneration);
    return run;
}
