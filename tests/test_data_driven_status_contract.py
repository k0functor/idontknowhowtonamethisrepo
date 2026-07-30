#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STATUS_PATH = ROOT / "data/statuses/combat_statuses.json"
MODIFIER_CPP = ROOT / "src/combat/ModifierSystem.cpp"
STATUS_CPP = ROOT / "src/statuses/StatusSystem.cpp"
PARSER_CPP = ROOT / "src/data/parsers/StatusDefinitionParser.cpp"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> None:
    statuses = json.loads(STATUS_PATH.read_text(encoding="utf-8"))
    by_id = {status["id"]: status for status in statuses}

    for status_id in ("strength", "dexterity", "ferocity", "weak", "vulnerable"):
        require(by_id[status_id].get("modifiers"), f"{status_id} must define its numeric modifiers in JSON")

    for stance in ("stance_flame", "stance_ash", "stance_smoke"):
        require(by_id[stance].get("exclusive_group") == "monk_stance",
                f"{stance} must use the data-driven monk_stance exclusive group")

    for status_id, log_id in (("poison", "poison_damage"), ("burn", "burn_damage")):
        triggers = by_id[status_id].get("triggers", [])
        require(len(triggers) == 1, f"{status_id} must have exactly one end-turn trigger")
        trigger = triggers[0]
        require(trigger.get("event") == "end_owner_turn", f"{status_id} trigger must run at end_owner_turn")
        require(trigger.get("effect") == "damage_hp", f"{status_id} trigger must deal HP damage")
        require(trigger.get("value_per_stack") == 1, f"{status_id} damage must scale with stacks")
        require(trigger.get("remove_stacks") == 1, f"{status_id} must lose one stack after triggering")
        require(trigger.get("log") == log_id, f"{status_id} must keep its combat log presentation")

    for status in statuses:
        require("end_turn_effect" not in status, "legacy end_turn_effect fields must be removed")
        require("decrease_after_trigger" not in status, "legacy decrease_after_trigger fields must be removed")

    modifier_source = MODIFIER_CPP.read_text(encoding="utf-8")
    for hardcoded_id in (
        '"strength"', '"dexterity"', '"ferocity"', '"weak"', '"vulnerable"',
        '"stance_flame"', '"stance_ash"', '"stance_smoke"',
        '"sadist_pleasure"', '"masochist_pain"'
    ):
        require(hardcoded_id not in modifier_source,
                f"ModifierSystem must not hard-code status id {hardcoded_id}")

    status_source = STATUS_CPP.read_text(encoding="utf-8")
    for hardcoded_id in ('"poison"', '"burn"', '"stance_flame"', '"stance_ash"', '"stance_smoke"'):
        require(hardcoded_id not in status_source,
                f"StatusSystem must not hard-code status id {hardcoded_id}")

    parser_source = PARSER_CPP.read_text(encoding="utf-8")
    require('optionalArray("modifiers")' in parser_source, "status parser must load modifier arrays")
    require('optionalArray("triggers")' in parser_source, "status parser must load trigger arrays")
    require('optionalString("exclusive_group"' in parser_source, "status parser must load exclusive groups")

    print(f"Validated {len(statuses)} data-driven status definitions")


if __name__ == "__main__":
    main()
