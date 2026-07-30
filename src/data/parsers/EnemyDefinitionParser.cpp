#include "EnemyDefinitionParser.hpp"

#include "data/JsonReader.hpp"
#include "data/parsers/EnemyActionDefinitionParser.hpp"
#include "data/parsers/EffectDefinitionParser.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_set>

namespace {
std::vector<EnemyActionDefinition> parseActions(
    const JsonReader& reader,
    const std::filesystem::path& sourcePath
) {
    const Json& actionsJson = reader.requiredArray("actions");

    std::vector<EnemyActionDefinition> actions;
    actions.reserve(actionsJson.size());

    for (const Json& actionJson : actionsJson) {
        actions.push_back(
            EnemyActionDefinitionParser::parse(actionJson, sourcePath)
        );
    }

    return actions;
}

std::vector<EffectDefinition> parseEffects(
    const Json& effectsJson,
    const std::filesystem::path& sourcePath
) {
    std::vector<EffectDefinition> effects;
    effects.reserve(effectsJson.size());
    for (const Json& effectJson : effectsJson) {
        effects.push_back(EffectDefinitionParser::parse(effectJson, sourcePath));
    }
    return effects;
}

std::vector<EnemyPhaseDefinition> parsePhases(
    const JsonReader& reader,
    const std::filesystem::path& sourcePath
) {
    const Json& phasesJson = reader.optionalArray("phases");
    std::vector<EnemyPhaseDefinition> phases;
    phases.reserve(phasesJson.size());

    int previousThreshold = 101;
    std::unordered_set<std::string> phaseIds;
    for (const Json& phaseJson : phasesJson) {
        JsonReader phaseReader(phaseJson, sourcePath);
        EnemyPhaseDefinition phase;
        phase.id = phaseReader.requiredString("id");
        phase.nameTextId = TextId(phaseReader.requiredString("name"));
        phase.activateBelowHpPercent = phaseReader.requiredInt("activate_below_hp_percent");
        phase.actionIds = phaseReader.requiredStringArray("action_ids");
        phase.onEnterEffects = parseEffects(phaseReader.optionalArray("on_enter_effects"), sourcePath);
        phase.playerTurnEffects = parseEffects(phaseReader.optionalArray("player_turn_effects"), sourcePath);
        phase.summonEnemyIds = phaseReader.optionalStringArray("summon_enemy_ids");
        phase.maximumAliveEnemies = phaseReader.optionalInt("maximum_alive_enemies", 3);

        if (phase.id.empty() || !phaseIds.insert(phase.id).second) {
            throw std::runtime_error(
                "JSON error in '" + sourcePath.string() + "': enemy phase ids must be non-empty and unique"
            );
        }
        if (phase.nameTextId.value.empty()) {
            throw std::runtime_error(
                "JSON error in '" + sourcePath.string() + "': enemy phase '" + phase.id + "' requires a name"
            );
        }
        if (phase.activateBelowHpPercent < 1 || phase.activateBelowHpPercent > 100 ||
            phase.activateBelowHpPercent >= previousThreshold) {
            throw std::runtime_error(
                "JSON error in '" + sourcePath.string() + "': enemy phases must use strictly descending HP thresholds in [1, 100]"
            );
        }
        if (phases.empty() && phase.activateBelowHpPercent != 100) {
            throw std::runtime_error(
                "JSON error in '" + sourcePath.string() + "': the first enemy phase must activate at 100% HP"
            );
        }
        if (phase.actionIds.empty()) {
            throw std::runtime_error(
                "JSON error in '" + sourcePath.string() + "': enemy phase '" + phase.id + "' must list at least one action"
            );
        }
        if (phase.maximumAliveEnemies < 1 || phase.maximumAliveEnemies > 3) {
            throw std::runtime_error(
                "JSON error in '" + sourcePath.string() + "': enemy phase '" + phase.id + "' maximum_alive_enemies must be in [1, 3]"
            );
        }

        previousThreshold = phase.activateBelowHpPercent;
        phases.push_back(std::move(phase));
    }

    return phases;
}
}

EnemyDefinition EnemyDefinitionParser::parse(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    EnemyDefinition definition;
    definition.id = EnemyId(reader.requiredString("id"));
    definition.nameTextId = TextId(reader.requiredString("name"));
    definition.maxHp = reader.requiredInt("max_hp");
    definition.startingBlock = reader.optionalInt("starting_block", 0);
    definition.role = enemyRoleFromString(reader.optionalString("role", "striker"));
    definition.actions = parseActions(reader, sourcePath);
    definition.phases = parsePhases(reader, sourcePath);

    if (definition.id.value.empty()) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': enemy id must not be empty"
        );
    }

    if (definition.maxHp <= 0) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': enemy max_hp must be positive"
        );
    }

    if (definition.startingBlock < 0) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': enemy starting_block must not be negative"
        );
    }

    if (definition.actions.empty()) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': enemy '" +
            definition.id.value + "' must have at least one action"
        );
    }

    return definition;
}
