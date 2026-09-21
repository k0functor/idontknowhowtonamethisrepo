#include "data/parsers/CardDefinitionParser.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
int failures = 0;

void check(const bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void testUnplayableCardMayHaveNoEffects() {
    const Json woundJson = {
        {"id", "integration_wound"},
        {"name", "card.integration_wound.name"},
        {"description", "card.integration_wound.description"},
        {"rarity", "special"},
        {"energy_cost", 0},
        {"gold_cost", 0},
        {"type", "status"},
        {"effects", Json::array()},
        {"keywords", Json::array({"unplayable"})}
    };

    const CardDefinition wound = CardDefinitionParser::parse(
        woundJson,
        "integration_wound.json"
    );
    check(wound.effects.empty(),
          "an unplayable status card may intentionally have no effects");

    Json invalidCard = woundJson;
    invalidCard["id"] = "integration_empty_playable";
    invalidCard["keywords"] = Json::array();

    bool rejected = false;
    try {
        static_cast<void>(CardDefinitionParser::parse(
            invalidCard,
            "integration_empty_playable.json"
        ));
    } catch (const std::runtime_error&) {
        rejected = true;
    }

    check(rejected,
          "a playable card without effects must still be rejected");
}

void testStatusPresenceScalingAliasIsParsed() {
    const Json cardJson = {
        {"id", "integration_stance_scaling"},
        {"name", "card.integration_stance_scaling.name"},
        {"description", "card.integration_stance_scaling.description"},
        {"rarity", "uncommon"},
        {"energy_cost", 1},
        {"gold_cost", 100},
        {"type", "attack"},
        {"effects", Json::array({
            {
                {"type", "damage"},
                {"target", "single_enemy"},
                {"value", {{"type", "fixed"}, {"amount", 5}}},
                {"scaling", {
                    {"status", "stance_flame"},
                    {"status_owner", "source"},
                    {"bonus_if_status_present", 7},
                    {"bonus_per_status_stack", 2},
                    {"maximum_bonus", 7}
                }}
            }
        })}
    };

    const CardDefinition card = CardDefinitionParser::parse(
        cardJson,
        "integration_stance_scaling.json"
    );

    check(card.effects.size() == 1, "stance-scaling card must parse one effect");
    check(card.effects.front().scaling.bonusIfStatusPresent == 7,
          "bonus_if_status_present must populate bonusIfStatusPresent");
    check(card.effects.front().scaling.bonusPerStatusStack == 2,
          "bonus_per_status_stack must populate bonusPerStatusStack");
}
}

int main() {
    testUnplayableCardMayHaveNoEffects();
    testStatusPresenceScalingAliasIsParsed();

    if (failures != 0) {
        std::cerr << failures << " card definition parser test(s) failed\n";
        return 1;
    }

    std::cout << "Card definition parser tests passed\n";
    return 0;
}
