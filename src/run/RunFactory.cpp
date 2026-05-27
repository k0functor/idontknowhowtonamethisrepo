#include "RunFactory.hpp"

#include "core/Random.hpp"
#include "run/RunMapGenerator.hpp"

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
        run.actorStates.push_back(RunActorState{actorId, actor.maxHp, actor.maxHp});
    }
    run.relicIds = archetype.startingRelicIds;

    run.deckCardIds.reserve(archetype.startingDeckCardIds.size());
    for (const std::string& cardId : archetype.startingDeckCardIds) {
        run.deckCardIds.emplace_back(cardId);
    }

    run.map = generator.generateActOneMap(mapRandom, mapGeneration);
    return run;
}
