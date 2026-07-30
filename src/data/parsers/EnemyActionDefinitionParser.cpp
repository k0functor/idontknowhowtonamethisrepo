#include "EnemyActionDefinitionParser.hpp"

#include "data/JsonReader.hpp"
#include "data/parsers/EffectDefinitionParser.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::optional<int> optionalInt(
    const JsonReader& reader,
    const std::string& key
) {
    if (!reader.has(key)) {
        return std::nullopt;
    }
    return reader.requiredInt(key);
}

std::optional<int> optionalPercent(
    const JsonReader& reader,
    const std::string& key,
    const std::filesystem::path& sourcePath,
    const std::string& actionId
) {
    const std::optional<int> value = optionalInt(reader, key);
    if (value.has_value() && (*value < 1 || *value > 100)) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': enemy action '" +
            actionId + "' field '" + key + "' must be in [1, 100]"
        );
    }
    return value;
}

void validateStatusIds(
    const std::vector<std::string>& ids,
    const std::filesystem::path& sourcePath,
    const std::string& actionId,
    const std::string& field
) {
    for (const std::string& id : ids) {
        if (id.empty()) {
            throw std::runtime_error(
                "JSON error in '" + sourcePath.string() + "': enemy action '" +
                actionId + "' field '" + field + "' must not contain empty status ids"
            );
        }
    }
}

EnemyActionCondition parseCondition(
    const Json& json,
    const std::filesystem::path& sourcePath,
    const std::string& actionId
) {
    JsonReader reader(json, sourcePath);
    EnemyActionCondition condition;

    condition.minTurn = reader.optionalInt("min_turn", 1);
    condition.maxTurn = optionalInt(reader, "max_turn");
    condition.selfHpBelowPercent = optionalPercent(
        reader, "self_hp_below_percent", sourcePath, actionId
    );
    condition.selfHpAbovePercent = optionalPercent(
        reader, "self_hp_above_percent", sourcePath, actionId
    );
    condition.anyPlayerHpBelowPercent = optionalPercent(
        reader, "any_player_hp_below_percent", sourcePath, actionId
    );
    condition.anyPlayerHpAbovePercent = optionalPercent(
        reader, "any_player_hp_above_percent", sourcePath, actionId
    );
    condition.anyOtherEnemyHpBelowPercent = optionalPercent(
        reader, "any_other_enemy_hp_below_percent", sourcePath, actionId
    );

    condition.minAliveEnemies = reader.optionalInt("min_alive_enemies", 1);
    condition.maxAliveEnemies = optionalInt(reader, "max_alive_enemies");

    condition.requiredSelfStatuses = reader.optionalStringArray("required_self_statuses");
    condition.forbiddenSelfStatuses = reader.optionalStringArray("forbidden_self_statuses");
    condition.requiredPlayerStatuses = reader.optionalStringArray("required_player_statuses");
    condition.forbiddenPlayerStatuses = reader.optionalStringArray("forbidden_player_statuses");

    if (condition.minTurn < 1) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': enemy action '" +
            actionId + "' condition min_turn must be at least 1"
        );
    }
    if (condition.maxTurn.has_value() && *condition.maxTurn < condition.minTurn) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': enemy action '" +
            actionId + "' condition max_turn must be at least min_turn"
        );
    }
    if (condition.minAliveEnemies < 1) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': enemy action '" +
            actionId + "' condition min_alive_enemies must be at least 1"
        );
    }
    if (condition.maxAliveEnemies.has_value() &&
        *condition.maxAliveEnemies < condition.minAliveEnemies) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': enemy action '" +
            actionId + "' condition max_alive_enemies must be at least min_alive_enemies"
        );
    }

    validateStatusIds(condition.requiredSelfStatuses, sourcePath, actionId, "required_self_statuses");
    validateStatusIds(condition.forbiddenSelfStatuses, sourcePath, actionId, "forbidden_self_statuses");
    validateStatusIds(condition.requiredPlayerStatuses, sourcePath, actionId, "required_player_statuses");
    validateStatusIds(condition.forbiddenPlayerStatuses, sourcePath, actionId, "forbidden_player_statuses");

    return condition;
}
} // namespace

EnemyActionDefinition EnemyActionDefinitionParser::parse(
    const Json& json,
    const std::filesystem::path& sourcePath
) {
    JsonReader reader(json, sourcePath);

    EnemyActionDefinition definition;
    definition.id = reader.requiredString("id");
    definition.intentType = enemyIntentTypeFromString(
        reader.requiredString("intent")
    );

    if (definition.id.empty()) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': enemy action id must not be empty"
        );
    }

    definition.weight = reader.optionalInt("weight", 1);
    definition.cooldown = reader.optionalInt("cooldown", 0);
    definition.maxConsecutiveUses = reader.optionalInt("max_consecutive_uses", 0);
    definition.condition = parseCondition(
        reader.optionalObject("conditions"),
        sourcePath,
        definition.id
    );

    if (definition.weight < 1) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': enemy action '" +
            definition.id + "' weight must be at least 1"
        );
    }
    if (definition.cooldown < 0) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': enemy action '" +
            definition.id + "' cooldown must not be negative"
        );
    }
    if (definition.maxConsecutiveUses < 0) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': enemy action '" +
            definition.id + "' max_consecutive_uses must not be negative"
        );
    }

    const Json& effectsJson = reader.requiredArray("effects");
    definition.effects.reserve(effectsJson.size());

    for (const Json& effectJson : effectsJson) {
        definition.effects.push_back(
            EffectDefinitionParser::parse(effectJson, sourcePath)
        );
    }

    if (definition.effects.empty()) {
        throw std::runtime_error(
            "JSON error in '" + sourcePath.string() + "': enemy action '" +
            definition.id + "' must have at least one effect"
        );
    }

    return definition;
}
