#include "RunFactory.hpp"

#include "run/RunMapGenerator.hpp"

RunState RunFactory::createRun(
    const PlayableArchetypeDefinition& archetype,
    const DifficultyDefinition& difficulty,
    const std::uint32_t seed
) const {
    RunMapGenerator generator;

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
    run.relicIds = archetype.startingRelicIds;

    run.deckCardIds.reserve(archetype.startingDeckCardIds.size());
    for (const std::string& cardId : archetype.startingDeckCardIds) {
        run.deckCardIds.emplace_back(cardId);
    }

    run.map = generator.generateTestMap();
    return run;
}
