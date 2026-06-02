#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Iterable

ROOT = Path(__file__).resolve().parents[1]
DATA_DIR = ROOT / "data"
CONFIG_DIR = ROOT / "config"
CARD_TYPES = {"attack", "skill", "power", "status", "curse"}
CARD_RARITIES = {"starter", "common", "uncommon", "rare", "special"}
CARD_KEYWORDS = {"exhaust", "retain", "ethereal", "innate", "unplayable"}
RELIC_RARITIES = {"starter", "common", "uncommon", "rare", "boss", "special"}
STATUS_TYPES = {"buff", "debuff", "neutral"}
STATUS_DURATION_RULES = {"persistent_combat", "decrease_end_of_owner_turn", "custom"}
CONSUMABLE_RARITIES = {"common", "uncommon", "rare", "special"}
EFFECT_TYPES = {
    "damage",
    "block",
    "heal",
    "draw_cards",
    "discard_cards",
    "apply_status",
    "gain_energy",
    "gain_stress",
    "lose_energy",
    "lose_stress",
    "lose_hp",
    "enter_stance",
    "summon_drone",
    "use_drone",
}
DRONE_EFFECT_TYPES = {"damage", "block", "heal", "draw_cards", "apply_status", "gain_energy"}

EFFECT_TARGETS = {
    "self",
    "single_enemy",
    "all_enemies",
    "random_enemy",
    "ally",
    "all_allies",
    "random_ally",
}
EFFECT_VALUE_TYPES = {"fixed", "dice"}
DICE_TYPES = {"d4", "d6", "d8", "d10", "d12", "d20"}
DICE_CORRUPTIONS = {"none", "cursed", "fire", "poison", "blood", "unstable"}
ENEMY_INTENTS = {"attack", "block", "buff", "debuff", "special", "unknown"}
GAME_EVENTS = {
    "combat_started",
    "combat_ended",
    "turn_started",
    "turn_ended",
    "card_played",
    "damage_dealt",
    "damage_taken",
    "block_gained",
    "healed",
    "status_applied",
    "enemy_killed",
    "reward_generated",
    "gold_gained",
    "relic_collected",
}
RELIC_MODIFIERS = {
    "outgoing_damage_add",
    "outgoing_damage_multiply",
    "block_add",
    "gold_reward_multiply",
}
RUN_EVENT_EFFECTS = {
    "gain_gold",
    "lose_gold",
    "gain_card",
    "gain_random_card",
    "gain_relic",
    "gain_random_relic",
    "gain_consumable",
    "gain_random_consumable",
    "remove_card",
    "remove_random_card",
    "gain_stress",
    "lose_stress",
    "lose_hp",
    "heal_all",
    "skip",
}
NODE_TYPES = {"combat", "elite", "boss", "chest", "event", "shop", "rest"}

NUMERIC_RUN_EVENT_EFFECTS = {
    "gain_gold",
    "lose_gold",
    "gain_stress",
    "lose_stress",
    "lose_hp",
    "heal_all",
}
CONTENT_RUN_EVENT_EFFECTS = {
    "gain_card": "card",
    "remove_card": "card",
    "gain_relic": "relic",
    "gain_consumable": "consumable",
}
ZERO_AMOUNT_RUN_EVENT_EFFECTS = {"remove_random_card", "skip"}


@dataclass
class IdIndex:
    items: dict[str, dict[str, Any]] = field(default_factory=dict)
    paths: dict[str, str] = field(default_factory=dict)

    def add(self, id_value: str, item: dict[str, Any], owner: str, errors: list[str]) -> None:
        if id_value in self.items:
            errors.append(
                f"{owner}: duplicate id '{id_value}' (already defined in {self.paths[id_value]})"
            )
            return
        self.items[id_value] = item
        self.paths[id_value] = owner

    def __contains__(self, id_value: str) -> bool:
        return id_value in self.items

    def __len__(self) -> int:
        return len(self.items)


@dataclass
class ProjectContent:
    actors: IdIndex = field(default_factory=IdIndex)
    archetypes: IdIndex = field(default_factory=IdIndex)
    cards: IdIndex = field(default_factory=IdIndex)
    consumables: IdIndex = field(default_factory=IdIndex)
    drones: IdIndex = field(default_factory=IdIndex)
    enemies: IdIndex = field(default_factory=IdIndex)
    encounters: IdIndex = field(default_factory=IdIndex)
    events: IdIndex = field(default_factory=IdIndex)
    relics: IdIndex = field(default_factory=IdIndex)
    statuses: IdIndex = field(default_factory=IdIndex)
    difficulties: IdIndex = field(default_factory=IdIndex)


class ProjectValidator:
    def __init__(self) -> None:
        self.errors: list[str] = []
        self.warnings: list[str] = []
        self.content = ProjectContent()
        self.loaded_json_files = 0
        self.effect_type_counts: Counter[str] = Counter()

    def error(self, owner: str, message: str) -> None:
        self.errors.append(f"{owner}: {message}")

    def warn(self, owner: str, message: str) -> None:
        self.warnings.append(f"{owner}: {message}")

    def load_json(self, path: Path) -> Any | None:
        try:
            with path.open("r", encoding="utf-8-sig") as file:
                data = json.load(file)
        except Exception as exc:  # noqa: BLE001 - this is a validator, not gameplay code.
            self.error(path.as_posix(), f"invalid JSON: {exc}")
            return None

        self.loaded_json_files += 1
        return data

    def validate_json_parse_smoke(self) -> None:
        for base_dir in (DATA_DIR, CONFIG_DIR):
            if not base_dir.exists():
                self.error(base_dir.relative_to(ROOT).as_posix(), "directory is missing")
                continue
            for path in sorted(base_dir.rglob("*.json")):
                self.load_json(path)

    def load_list_file(self, relative_path: str, index: IdIndex, required_fields: Iterable[str]) -> list[dict[str, Any]]:
        path = ROOT / relative_path
        data = self.load_json(path)
        if data is None:
            return []
        if not isinstance(data, list):
            self.error(relative_path, "root must be an array")
            return []

        result: list[dict[str, Any]] = []
        for item_index, item in enumerate(data):
            owner = f"{relative_path}[{item_index}]"
            if not isinstance(item, dict):
                self.error(owner, "item must be an object")
                continue
            id_value = item.get("id")
            if not isinstance(id_value, str) or not id_value:
                self.error(owner, "id must be a non-empty string")
                continue
            for field_name in required_fields:
                if field_name not in item:
                    self.error(f"{relative_path}:{id_value}", f"missing required field '{field_name}'")
            index.add(id_value, item, f"{relative_path}:{id_value}", self.errors)
            result.append(item)
        return result

    def load_indexed_content(self) -> None:
        self.load_list_file(
            "data/actors/player_actors.json",
            self.content.actors,
            ["name", "description", "max_hp", "starting_energy"],
        )
        self.load_list_file(
            "data/archetypes/playable_archetypes.json",
            self.content.archetypes,
            [
                "name",
                "short_description",
                "details_description",
                "unique_mechanic",
                "actors",
                "reward_card_pools",
                "starting_deck",
                "starting_relics",
                "starting_gold",
                "mechanic_id",
                "selection_order",
                "is_available",
            ],
        )
        for path in sorted((DATA_DIR / "cards").glob("*.json")):
            self.load_list_file(
                path.relative_to(ROOT).as_posix(),
                self.content.cards,
                ["name", "description", "rarity", "energy_cost", "gold_cost", "type", "effects"],
            )
        self.load_list_file(
            "data/consumables/potions.json",
            self.content.consumables,
            ["name", "description", "rarity", "gold_cost", "effects"],
        )
        self.load_list_file(
            "data/drones/combat_drones.json",
            self.content.drones,
            ["name", "description", "passive", "active"],
        )
        for path in sorted((DATA_DIR / "enemies").glob("*.json")):
            self.load_list_file(
                path.relative_to(ROOT).as_posix(),
                self.content.enemies,
                ["name", "max_hp", "starting_block", "actions"],
            )
        self.load_list_file(
            "data/events/run_events.json",
            self.content.events,
            ["title", "description", "choices"],
        )
        for path in sorted((DATA_DIR / "relics").glob("*.json")):
            self.load_list_file(
                path.relative_to(ROOT).as_posix(),
                self.content.relics,
                ["name", "description", "rarity"],
            )
        self.load_list_file(
            "data/statuses/combat_statuses.json",
            self.content.statuses,
            ["name", "description", "type", "duration_rule"],
        )
        self.load_list_file(
            "data/run/difficulties.json",
            self.content.difficulties,
            ["name", "description", "enemy_hp_multiplier", "enemy_damage_multiplier", "gold_multiplier"],
        )
        self.load_encounters()

    def load_encounters(self) -> None:
        relative_path = "data/encounters/act1_encounters.json"
        data = self.load_json(ROOT / relative_path)
        if data is None:
            return
        if not isinstance(data, dict):
            self.error(relative_path, "root must be an object")
            return
        pools = data.get("pools")
        if not isinstance(pools, dict):
            self.error(relative_path, "pools must be an object")
            return
        for pool_name, encounters in pools.items():
            owner = f"{relative_path}:pools.{pool_name}"
            if pool_name not in {"combat", "elite", "boss"}:
                self.error(owner, "unknown encounter pool")
            if not isinstance(encounters, list):
                self.error(owner, "pool must be an array")
                continue
            for encounter_index, encounter in enumerate(encounters):
                encounter_owner = f"{owner}[{encounter_index}]"
                if not isinstance(encounter, dict):
                    self.error(encounter_owner, "encounter must be an object")
                    continue
                id_value = encounter.get("id")
                if not isinstance(id_value, str) or not id_value:
                    self.error(encounter_owner, "id must be a non-empty string")
                    continue
                self.content.encounters.add(id_value, encounter, f"{relative_path}:{id_value}", self.errors)

    def expect_enum(self, owner: str, field_name: str, value: Any, allowed: set[str]) -> str | None:
        if not isinstance(value, str):
            self.error(owner, f"{field_name} must be a string")
            return None
        if value not in allowed:
            self.error(owner, f"unknown {field_name} '{value}'")
            return None
        return value

    def expect_int(self, owner: str, field_name: str, value: Any, *, minimum: int | None = None) -> int | None:
        if not isinstance(value, int) or isinstance(value, bool):
            self.error(owner, f"{field_name} must be an integer")
            return None
        if minimum is not None and value < minimum:
            self.error(owner, f"{field_name} must be >= {minimum}")
        return value

    def validate_effect_value(self, owner: str, value: Any) -> None:
        if not isinstance(value, dict):
            self.error(owner, "value must be an object")
            return
        value_type = self.expect_enum(owner, "value.type", value.get("type"), EFFECT_VALUE_TYPES)
        if value_type == "fixed":
            self.expect_int(owner, "value.amount", value.get("amount"))
        elif value_type == "dice":
            self.expect_int(owner, "value.count", value.get("count", 1), minimum=1)
            self.expect_enum(owner, "value.die", value.get("die"), DICE_TYPES)
            if "bonus" in value:
                self.expect_int(owner, "value.bonus", value.get("bonus"))

    def validate_effect_list(self, owner: str, effects: Any) -> None:
        if effects is None:
            return
        if not isinstance(effects, list):
            self.error(owner, "effects must be an array")
            return
        for index, effect in enumerate(effects):
            effect_owner = f"{owner}.effects[{index}]"
            if not isinstance(effect, dict):
                self.error(effect_owner, "effect must be an object")
                continue
            effect_type = self.expect_enum(effect_owner, "type", effect.get("type"), EFFECT_TYPES)
            if effect_type is None:
                continue
            self.effect_type_counts[effect_type] += 1

            self.expect_enum(effect_owner, "target", effect.get("target", "self"), EFFECT_TARGETS)
            if "value" in effect:
                self.validate_effect_value(effect_owner, effect["value"])
            if "repeat_count" in effect:
                self.expect_int(effect_owner, "repeat_count", effect.get("repeat_count"), minimum=1)
            if "times" in effect:
                self.expect_int(effect_owner, "times", effect.get("times"), minimum=1)

            status_id = effect.get("status")
            if effect_type == "apply_status" or effect_type == "enter_stance":
                if not isinstance(status_id, str) or not status_id:
                    self.error(effect_owner, "status effect must define a non-empty status")
                elif status_id not in self.content.statuses:
                    self.error(effect_owner, f"references unknown status '{status_id}'")
            elif effect_type == "summon_drone":
                if not isinstance(status_id, str) or not status_id:
                    self.error(effect_owner, "summon_drone must define a non-empty drone id in status")
                elif status_id not in self.content.drones:
                    self.error(effect_owner, f"references unknown drone '{status_id}'")
            elif status_id is not None:
                self.warn(effect_owner, f"has unused status field '{status_id}' for effect type '{effect_type}'")

            if effect_type == "use_drone":
                target = effect.get("target", "self")
                if target != "self":
                    self.error(effect_owner, "use_drone must target self")
                if status_id is not None:
                    self.error(effect_owner, "use_drone activates the oldest available drone and must not name a drone")

    def validate_cards(self) -> None:
        for card_id, card in self.content.cards.items.items():
            owner = self.content.cards.paths[card_id]
            self.expect_enum(owner, "rarity", card.get("rarity"), CARD_RARITIES)
            self.expect_enum(owner, "type", card.get("type"), CARD_TYPES)
            self.expect_int(owner, "energy_cost", card.get("energy_cost"), minimum=0)
            self.expect_int(owner, "gold_cost", card.get("gold_cost"), minimum=0)
            if "owner_actor" in card and card["owner_actor"] not in self.content.actors:
                self.error(owner, f"references unknown owner_actor '{card['owner_actor']}'")
            for keyword in card.get("keywords", []):
                self.expect_enum(owner, "keyword", keyword, CARD_KEYWORDS)
            if "dice_corruption" in card:
                corruption = card["dice_corruption"]
                if isinstance(corruption, str):
                    self.expect_enum(owner, "dice_corruption", corruption, DICE_CORRUPTIONS)
                elif isinstance(corruption, dict):
                    self.expect_enum(owner, "dice_corruption.type", corruption.get("type"), DICE_CORRUPTIONS)
                else:
                    self.error(owner, "dice_corruption must be a string or an object")
            self.validate_effect_list(owner, card.get("effects"))

            upgrade = card.get("upgrade")
            if upgrade is not None:
                if not isinstance(upgrade, dict):
                    self.error(owner, "upgrade must be an object")
                else:
                    if "energy_cost" in upgrade:
                        self.expect_int(owner, "upgrade.energy_cost", upgrade.get("energy_cost"), minimum=0)
                    if "gold_cost" in upgrade:
                        self.expect_int(owner, "upgrade.gold_cost", upgrade.get("gold_cost"), minimum=0)
                    if "effects" in upgrade:
                        self.validate_effect_list(f"{owner}.upgrade", upgrade.get("effects"))

    def validate_actors_and_archetypes(self) -> None:
        for actor_id, actor in self.content.actors.items.items():
            owner = self.content.actors.paths[actor_id]
            self.expect_int(owner, "max_hp", actor.get("max_hp"), minimum=1)
            self.expect_int(owner, "starting_energy", actor.get("starting_energy"), minimum=0)
            for relic_id in actor.get("starting_relics", []):
                if relic_id not in self.content.relics:
                    self.error(owner, f"references unknown starting relic '{relic_id}'")

        for archetype_id, archetype in self.content.archetypes.items.items():
            owner = self.content.archetypes.paths[archetype_id]
            actor_ids = archetype.get("actors", [])
            if not isinstance(actor_ids, list) or not actor_ids:
                self.error(owner, "actors must be a non-empty array")
            else:
                for actor_id in actor_ids:
                    if actor_id not in self.content.actors:
                        self.error(owner, f"references unknown actor '{actor_id}'")

            reward_pools = archetype.get("reward_card_pools", [])
            if not isinstance(reward_pools, list) or not reward_pools:
                self.error(owner, "reward_card_pools must be a non-empty array")
            else:
                for pool_id in reward_pools:
                    if pool_id not in self.content.actors:
                        self.error(owner, f"references unknown reward card pool '{pool_id}'")

            for card_id in archetype.get("starting_deck", []):
                if card_id not in self.content.cards:
                    self.error(owner, f"references unknown starting card '{card_id}'")
            for relic_id in archetype.get("starting_relics", []):
                if relic_id not in self.content.relics:
                    self.error(owner, f"references unknown starting relic '{relic_id}'")
            for consumable_id in archetype.get("starting_consumables", []):
                if consumable_id not in self.content.consumables:
                    self.error(owner, f"references unknown starting consumable '{consumable_id}'")

            if archetype.get("is_available") is True:
                reward_candidates = [
                    card
                    for card in self.content.cards.items.values()
                    if card.get("owner_actor") in reward_pools
                    and self.card_can_appear_as_reward(card)
                ]
                if not reward_candidates:
                    self.error(owner, "available archetype has no non-starter card reward candidates")

    def card_can_appear_as_reward(self, card: dict[str, Any]) -> bool:
        return card.get("type") not in {"status", "curse"} and card.get("rarity") not in {"starter", "special"}

    def relic_can_appear_as_reward(self, relic: dict[str, Any]) -> bool:
        return relic.get("rarity") not in {"starter", "special"}

    def validate_consumables_drones_statuses_relics(self) -> None:
        for status_id, status in self.content.statuses.items.items():
            owner = self.content.statuses.paths[status_id]
            self.expect_enum(owner, "type", status.get("type"), STATUS_TYPES)
            self.expect_enum(owner, "duration_rule", status.get("duration_rule"), STATUS_DURATION_RULES)
            if status.get("end_turn_effect") not in (None, "poison_damage"):
                self.error(owner, f"unknown end_turn_effect '{status.get('end_turn_effect')}'")

        for consumable_id, consumable in self.content.consumables.items.items():
            owner = self.content.consumables.paths[consumable_id]
            self.expect_enum(owner, "rarity", consumable.get("rarity"), CONSUMABLE_RARITIES)
            self.expect_int(owner, "gold_cost", consumable.get("gold_cost"), minimum=0)
            self.validate_effect_list(owner, consumable.get("effects"))

        for drone_id, drone in self.content.drones.items.items():
            owner = self.content.drones.paths[drone_id]
            for action_name in ("passive", "active"):
                action = drone.get(action_name)
                action_owner = f"{owner}.{action_name}"
                if not isinstance(action, dict):
                    self.error(action_owner, "must be an object")
                    continue
                self.validate_effect_list(action_owner, action.get("effects"))
                for effect_index, effect in enumerate(action.get("effects", [])):
                    if not isinstance(effect, dict):
                        continue
                    effect_type = effect.get("type")
                    if effect_type in EFFECT_TYPES and effect_type not in DRONE_EFFECT_TYPES:
                        self.error(
                            f"{action_owner}.effects[{effect_index}]",
                            f"effect type '{effect_type}' is not supported by DroneSystem",
                        )

        for relic_id, relic in self.content.relics.items.items():
            owner = self.content.relics.paths[relic_id]
            self.expect_enum(owner, "rarity", relic.get("rarity"), RELIC_RARITIES)
            for index, modifier in enumerate(relic.get("modifiers", [])):
                modifier_owner = f"{owner}.modifiers[{index}]"
                if not isinstance(modifier, dict):
                    self.error(modifier_owner, "modifier must be an object")
                    continue
                self.expect_enum(modifier_owner, "type", modifier.get("type"), RELIC_MODIFIERS)
                self.expect_int(modifier_owner, "priority", modifier.get("priority", 0))
                numeric_value = modifier.get("amount", modifier.get("multiplier"))
                if not isinstance(numeric_value, (int, float)) or isinstance(numeric_value, bool):
                    self.error(modifier_owner, "amount or multiplier must be numeric")
            for index, trigger in enumerate(relic.get("triggers", [])):
                trigger_owner = f"{owner}.triggers[{index}]"
                if not isinstance(trigger, dict):
                    self.error(trigger_owner, "trigger must be an object")
                    continue
                self.expect_enum(trigger_owner, "event", trigger.get("event"), GAME_EVENTS)
                if "status" in trigger and trigger["status"] not in self.content.statuses:
                    self.error(trigger_owner, f"references unknown status filter '{trigger['status']}'")
                if "card_type" in trigger:
                    self.expect_enum(trigger_owner, "card_type", trigger.get("card_type"), CARD_TYPES)
                    if trigger.get("event") != "card_played":
                        self.error(trigger_owner, "card_type filter is supported only for card_played")
                if trigger.get("source_side", "any") not in {"any", "player", "enemy"}:
                    self.error(trigger_owner, "source_side must be one of: any, player, enemy")
                if "every_n_turns" in trigger:
                    self.expect_int(trigger_owner, "every_n_turns", trigger.get("every_n_turns"), minimum=0)
                if "min_amount" in trigger:
                    self.expect_int(trigger_owner, "min_amount", trigger.get("min_amount"), minimum=0)
                self.validate_effect_list(trigger_owner, trigger.get("effects"))

    def validate_enemies_and_encounters(self) -> None:
        for enemy_id, enemy in self.content.enemies.items.items():
            owner = self.content.enemies.paths[enemy_id]
            self.expect_int(owner, "max_hp", enemy.get("max_hp"), minimum=1)
            self.expect_int(owner, "starting_block", enemy.get("starting_block"), minimum=0)
            actions = enemy.get("actions")
            if not isinstance(actions, list) or not actions:
                self.error(owner, "actions must be a non-empty array")
                continue
            action_ids: set[str] = set()
            for index, action in enumerate(actions):
                action_owner = f"{owner}.actions[{index}]"
                if not isinstance(action, dict):
                    self.error(action_owner, "action must be an object")
                    continue
                action_id = action.get("id")
                if not isinstance(action_id, str) or not action_id:
                    self.error(action_owner, "id must be a non-empty string")
                elif action_id in action_ids:
                    self.error(action_owner, f"duplicate action id '{action_id}'")
                else:
                    action_ids.add(action_id)
                self.expect_enum(action_owner, "intent", action.get("intent"), ENEMY_INTENTS)
                self.validate_effect_list(action_owner, action.get("effects"))

        for encounter_id, encounter in self.content.encounters.items.items():
            owner = self.content.encounters.paths[encounter_id]
            enemies = encounter.get("enemies")
            if not isinstance(enemies, list) or not enemies:
                self.error(owner, "enemies must be a non-empty array")
            else:
                for enemy_id in enemies:
                    if enemy_id not in self.content.enemies:
                        self.error(owner, f"references unknown enemy '{enemy_id}'")
            self.expect_int(owner, "weight", encounter.get("weight", 1), minimum=1)
            min_layer = self.expect_int(owner, "min_layer", encounter.get("min_layer", 0), minimum=0)
            max_layer = self.expect_int(owner, "max_layer", encounter.get("max_layer", 0), minimum=0)
            if min_layer is not None and max_layer is not None and min_layer > max_layer:
                self.error(owner, "min_layer must be <= max_layer")

    def validate_events(self) -> None:
        has_card_rewards = any(self.card_can_appear_as_reward(card) for card in self.content.cards.items.values())
        has_relic_rewards = any(self.relic_can_appear_as_reward(relic) for relic in self.content.relics.items.values())
        has_consumables = len(self.content.consumables) > 0

        for event_id, event in self.content.events.items.items():
            owner = self.content.events.paths[event_id]
            choices = event.get("choices")
            if not isinstance(choices, list) or not choices:
                self.error(owner, "choices must be a non-empty array")
                continue
            for choice_index, choice in enumerate(choices):
                choice_owner = f"{owner}.choices[{choice_index}]"
                if not isinstance(choice, dict):
                    self.error(choice_owner, "choice must be an object")
                    continue
                requirements = choice.get("requirements", {})
                if requirements is not None and not isinstance(requirements, dict):
                    self.error(choice_owner, "requirements must be an object")
                    requirements = {}
                for key in ("has_relic", "missing_relic"):
                    relic_id = requirements.get(key)
                    if relic_id is not None and relic_id not in self.content.relics:
                        self.error(choice_owner, f"{key} references unknown relic '{relic_id}'")
                for key in ("has_relics", "missing_relics"):
                    for relic_id in requirements.get(key, []):
                        if relic_id not in self.content.relics:
                            self.error(choice_owner, f"{key} references unknown relic '{relic_id}'")
                for key in ("has_card", "missing_card"):
                    card_id = requirements.get(key)
                    if card_id is not None and card_id not in self.content.cards:
                        self.error(choice_owner, f"{key} references unknown card '{card_id}'")
                for key in ("has_cards", "missing_cards"):
                    for card_id in requirements.get(key, []):
                        if card_id not in self.content.cards:
                            self.error(choice_owner, f"{key} references unknown card '{card_id}'")
                for numeric_key in ("min_gold", "min_hp", "min_deck_size"):
                    if numeric_key in requirements:
                        self.expect_int(choice_owner, numeric_key, requirements.get(numeric_key), minimum=0)

                effects = choice.get("effects", [])
                if not isinstance(effects, list):
                    self.error(choice_owner, "effects must be an array")
                    continue
                for effect_index, effect in enumerate(effects):
                    effect_owner = f"{choice_owner}.effects[{effect_index}]"
                    if not isinstance(effect, dict):
                        self.error(effect_owner, "event effect must be an object")
                        continue
                    effect_type = self.expect_enum(effect_owner, "type", effect.get("type"), RUN_EVENT_EFFECTS)
                    if effect_type is None:
                        continue
                    if effect_type in NUMERIC_RUN_EVENT_EFFECTS:
                        self.expect_int(effect_owner, "amount", effect.get("amount"), minimum=1)
                    elif effect_type in ZERO_AMOUNT_RUN_EVENT_EFFECTS and effect.get("amount", 0) != 0:
                        self.error(effect_owner, "amount must be omitted or zero")
                    elif effect_type in CONTENT_RUN_EVENT_EFFECTS:
                        content_id = (
                            effect.get("content_id")
                            or effect.get("id")
                            or effect.get("card_id")
                            or effect.get("relic_id")
                            or effect.get("consumable_id")
                        )
                        if not isinstance(content_id, str) or not content_id:
                            self.error(effect_owner, "content effect must define a content id")
                        elif CONTENT_RUN_EVENT_EFFECTS[effect_type] == "card" and content_id not in self.content.cards:
                            self.error(effect_owner, f"references unknown card '{content_id}'")
                        elif CONTENT_RUN_EVENT_EFFECTS[effect_type] == "relic" and content_id not in self.content.relics:
                            self.error(effect_owner, f"references unknown relic '{content_id}'")
                        elif CONTENT_RUN_EVENT_EFFECTS[effect_type] == "consumable" and content_id not in self.content.consumables:
                            self.error(effect_owner, f"references unknown consumable '{content_id}'")
                    elif effect_type == "gain_random_card" and not has_card_rewards:
                        self.error(effect_owner, "random card reward pool is empty")
                    elif effect_type == "gain_random_relic" and not has_relic_rewards:
                        self.error(effect_owner, "random relic reward pool is empty")
                    elif effect_type == "gain_random_consumable" and not has_consumables:
                        self.error(effect_owner, "random consumable pool is empty")

    def validate_reward_and_shop_tables(self) -> None:
        reward_data = self.load_json(ROOT / "data/rewards/reward_tables.json")
        if isinstance(reward_data, dict):
            merchant_gold_multiplier = reward_data.get("merchant_gold_multiplier")
            if not isinstance(merchant_gold_multiplier, (int, float)) or isinstance(merchant_gold_multiplier, bool) or merchant_gold_multiplier < 1.0:
                self.error("data/rewards/reward_tables.json", "merchant_gold_multiplier must be a number >= 1.0")
            nodes = reward_data.get("nodes")
            if not isinstance(nodes, dict):
                self.error("data/rewards/reward_tables.json", "nodes must be an object")
            else:
                for node_type in NODE_TYPES:
                    if node_type not in nodes:
                        self.error("data/rewards/reward_tables.json", f"missing reward node '{node_type}'")
                for node_type, node in nodes.items():
                    owner = f"data/rewards/reward_tables.json:nodes.{node_type}"
                    if node_type not in NODE_TYPES:
                        self.error(owner, "unknown reward node type")
                    if not isinstance(node, dict):
                        self.error(owner, "node reward tuning must be an object")
                        continue
                    self.expect_int(owner, "gold", node.get("gold", 0), minimum=0)
                    self.expect_int(owner, "card_choices", node.get("card_choices", 0), minimum=0)
                    self.expect_int(owner, "consumable_chance_percent", node.get("consumable_chance_percent", 0), minimum=0)
                    if node.get("consumable_chance_percent", 0) > 100:
                        self.error(owner, "consumable_chance_percent must be <= 100")
                    if node.get("offer_cards") is True and not any(self.card_can_appear_as_reward(card) for card in self.content.cards.items.values()):
                        self.error(owner, "offers cards but the card reward pool is empty")
                    if node.get("guaranteed_relic") is True and not any(self.relic_can_appear_as_reward(relic) for relic in self.content.relics.items.values()):
                        self.error(owner, "guarantees a relic but the relic reward pool is empty")

        shop_data = self.load_json(ROOT / "data/shop/shop_tables.json")
        if isinstance(shop_data, dict):
            owner = "data/shop/shop_tables.json"
            for key in (
                "card_offers",
                "relic_offers",
                "consumable_offers",
                "card_removal_price",
                "minimum_card_price",
                "minimum_consumable_price",
                "merchant_rest_card_offers",
                "merchant_rest_max_card_purchases",
            ):
                self.expect_int(owner, key, shop_data.get(key), minimum=0)
            price_multiplier = shop_data.get("merchant_rest_card_price_multiplier")
            if not isinstance(price_multiplier, (int, float)) or isinstance(price_multiplier, bool) or price_multiplier < 0:
                self.error(owner, "merchant_rest_card_price_multiplier must be a non-negative number")
            if isinstance(shop_data.get("merchant_rest_card_offers"), int) and isinstance(shop_data.get("merchant_rest_max_card_purchases"), int):
                if shop_data["merchant_rest_max_card_purchases"] > shop_data["merchant_rest_card_offers"]:
                    self.error(owner, "merchant_rest_max_card_purchases cannot exceed merchant_rest_card_offers")
                merchant_is_available = any(
                    archetype.get("id") == "merchant" and archetype.get("is_available") is True
                    for archetype in self.content.archetypes.items.values()
                )
                if merchant_is_available:
                    if shop_data["merchant_rest_card_offers"] <= 0:
                        self.error(owner, "merchant_rest_card_offers must be positive while merchant is available")
                    if shop_data["merchant_rest_max_card_purchases"] <= 0:
                        self.error(owner, "merchant_rest_max_card_purchases must be positive while merchant is available")
            relic_prices = shop_data.get("relic_prices")
            if not isinstance(relic_prices, dict):
                self.error(owner, "relic_prices must be an object")
            else:
                for rarity, price in relic_prices.items():
                    self.expect_enum(f"{owner}.relic_prices", "rarity", rarity, RELIC_RARITIES)
                    self.expect_int(f"{owner}.relic_prices.{rarity}", "price", price, minimum=0)

    def validate_map_config(self) -> None:
        relative_path = "data/run/acts/act1.json"
        data = self.load_json(ROOT / relative_path)
        if not isinstance(data, dict):
            self.error(relative_path, "root must be an object")
            return
        if not isinstance(data.get("id"), str) or not data.get("id"):
            self.error(relative_path, "id must be a non-empty string")

        layer_counts = data.get("layer_node_counts")
        if isinstance(layer_counts, list):
            if len(layer_counts) < 4:
                self.error(relative_path, "layer_node_counts must define at least 4 layers")
            for index, count in enumerate(layer_counts):
                self.expect_int(relative_path, f"layer_node_counts[{index}]", count, minimum=1)
            layer_count = len(layer_counts)
        else:
            layer_count = self.expect_int(relative_path, "layer_count", data.get("layer_count", 8), minimum=4) or 0

        first_middle_layer = 1
        last_middle_layer = layer_count - 3
        pre_boss_layer = layer_count - 2
        boss_layer = layer_count - 1
        if last_middle_layer < first_middle_layer:
            self.error(relative_path, "map leaves no middle layers")
            return

        if isinstance(layer_counts, list):
            if layer_counts[0] != 1:
                self.error(relative_path, "layer 0 must contain exactly one starting combat room")
            if layer_counts[boss_layer] != 1:
                self.error(relative_path, "boss layer must contain exactly one boss room")
            if layer_counts[pre_boss_layer] <= 0:
                self.error(relative_path, "pre-boss rest layer must not be empty")

        def range_capacity(min_layer: int, max_layer: int) -> int:
            if isinstance(layer_counts, list):
                return sum(layer_counts[min_layer : max_layer + 1])
            return (max_layer - min_layer + 1) * int(data.get("middle_max_nodes", 4))

        special_counts: dict[str, int] = {}

        for key in ("combat_weight", "event_weight", "extra_connection_chance", "question_mark_combat_chance"):
            if key in data:
                self.expect_int(relative_path, key, data.get(key), minimum=0)
        for key in ("extra_connection_chance", "question_mark_combat_chance"):
            if data.get(key, 0) > 100:
                self.error(relative_path, f"{key} must be <= 100")

        for key in ("shop", "chests"):
            config = data.get(key, {})
            owner = f"{relative_path}:{key}"
            if not isinstance(config, dict):
                self.error(owner, "must be an object")
                continue
            count = self.expect_int(owner, "count", config.get("count", 0), minimum=0)
            min_layer = self.expect_int(owner, "min_layer", config.get("min_layer", first_middle_layer), minimum=first_middle_layer)
            max_layer = self.expect_int(owner, "max_layer", config.get("max_layer", last_middle_layer), minimum=first_middle_layer)
            if max_layer is not None and max_layer > last_middle_layer:
                self.error(owner, f"max_layer must be <= {last_middle_layer}")
            if min_layer is not None and max_layer is not None and min_layer > max_layer:
                self.error(owner, "min_layer must be <= max_layer")
            if count is not None and min_layer is not None and max_layer is not None and min_layer <= max_layer and max_layer <= last_middle_layer:
                capacity = range_capacity(min_layer, max_layer)
                if count > capacity:
                    self.error(owner, f"count cannot fit into the configured layer range with capacity {capacity}")
                special_counts[key] = count

            if "full_layer" in config and not isinstance(config.get("full_layer"), bool):
                self.error(owner, "full_layer must be a boolean")

        chests = data.get("chests", {})
        if isinstance(chests, dict) and chests.get("full_layer") is True:
            owner = f"{relative_path}:chests"
            count = chests.get("count", 0)
            min_layer = chests.get("min_layer", first_middle_layer)
            max_layer = chests.get("max_layer", last_middle_layer)
            if not isinstance(layer_counts, list):
                self.error(owner, "full_layer requires explicit layer_node_counts")
            if not isinstance(count, int) or isinstance(count, bool) or count <= 0:
                self.error(owner, "full_layer requires count > 0")
            if min_layer != max_layer:
                self.error(owner, "full_layer requires min_layer == max_layer")
            elif isinstance(layer_counts, list) and isinstance(count, int) and not isinstance(count, bool):
                layer_width = layer_counts[min_layer]
                if count != layer_width:
                    self.error(owner, f"full_layer count must equal layer {min_layer} width ({layer_width})")

        elites = data.get("elites", {})
        if isinstance(elites, dict):
            owner = f"{relative_path}:elites"
            minimum = self.expect_int(owner, "min", elites.get("min", 0), minimum=0)
            maximum = self.expect_int(owner, "max", elites.get("max", 0), minimum=0)
            min_layer = self.expect_int(owner, "min_layer", elites.get("min_layer", first_middle_layer), minimum=first_middle_layer)
            max_layer = self.expect_int(owner, "max_layer", elites.get("max_layer", last_middle_layer), minimum=first_middle_layer)
            if minimum is not None and maximum is not None and minimum > maximum:
                self.error(owner, "min must be <= max")
            if max_layer is not None and max_layer > last_middle_layer:
                self.error(owner, f"max_layer must be <= {last_middle_layer}")
            if min_layer is not None and max_layer is not None and min_layer > max_layer:
                self.error(owner, "min_layer must be <= max_layer")
            if maximum is not None and min_layer is not None and max_layer is not None and min_layer <= max_layer and max_layer <= last_middle_layer:
                capacity = range_capacity(min_layer, max_layer)
                if maximum > capacity:
                    self.error(owner, f"max cannot fit into the configured layer range with capacity {capacity}")
                special_counts["elites"] = maximum
        else:
            self.error(f"{relative_path}:elites", "must be an object")

        events = data.get("events")
        if events is not None:
            owner = f"{relative_path}:events"
            if not isinstance(events, dict):
                self.error(owner, "must be an object")
            else:
                minimum = self.expect_int(owner, "min", events.get("min", 0), minimum=0)
                maximum = self.expect_int(owner, "max", events.get("max", 0), minimum=0)
                min_layer = self.expect_int(owner, "min_layer", events.get("min_layer", first_middle_layer), minimum=first_middle_layer)
                max_layer = self.expect_int(owner, "max_layer", events.get("max_layer", last_middle_layer), minimum=first_middle_layer)
                if minimum is not None and maximum is not None and minimum > maximum:
                    self.error(owner, "min must be <= max")
                if max_layer is not None and max_layer > last_middle_layer:
                    self.error(owner, f"max_layer must be <= {last_middle_layer}")
                if min_layer is not None and max_layer is not None and min_layer > max_layer:
                    self.error(owner, "min_layer must be <= max_layer")
                if maximum is not None and min_layer is not None and max_layer is not None and min_layer <= max_layer and max_layer <= last_middle_layer:
                    capacity = range_capacity(min_layer, max_layer)
                    if maximum > capacity:
                        self.error(owner, f"max cannot fit into the configured layer range with capacity {capacity}")
                    special_counts["events"] = maximum

        middle_capacity = range_capacity(first_middle_layer, last_middle_layer)
        if sum(special_counts.values()) > middle_capacity:
            self.error(relative_path, f"special node requests cannot fit into middle layers with capacity {middle_capacity}")

    def validate_difficulties(self) -> None:
        for difficulty_id, difficulty in self.content.difficulties.items.items():
            owner = self.content.difficulties.paths[difficulty_id]
            for key in ("enemy_hp_multiplier", "enemy_damage_multiplier", "gold_multiplier"):
                value = difficulty.get(key)
                if not isinstance(value, (int, float)) or isinstance(value, bool):
                    self.error(owner, f"{key} must be numeric")
                elif value <= 0:
                    self.error(owner, f"{key} must be positive")

    def run(self) -> int:
        self.validate_json_parse_smoke()
        self.load_indexed_content()
        if self.errors:
            self.print_result()
            return 1

        self.validate_consumables_drones_statuses_relics()
        self.validate_cards()
        self.validate_actors_and_archetypes()
        self.validate_enemies_and_encounters()
        self.validate_events()
        self.validate_reward_and_shop_tables()
        self.validate_map_config()
        self.validate_difficulties()
        self.print_result()
        return 1 if self.errors else 0

    def print_result(self) -> None:
        if self.errors:
            print("Project content validation failed:")
            for error in self.errors:
                print(f" - {error}")
        else:
            print(
                "Project content OK: "
                f"{len(self.content.cards)} cards, "
                f"{len(self.content.enemies)} enemies, "
                f"{len(self.content.encounters)} encounters, "
                f"{len(self.content.relics)} relics, "
                f"{len(self.content.consumables)} consumables, "
                f"{len(self.content.events)} events, "
                f"{len(self.content.archetypes)} archetypes"
            )
            if self.effect_type_counts:
                most_common = ", ".join(
                    f"{effect_type}={count}"
                    for effect_type, count in sorted(self.effect_type_counts.items())
                )
                print(f"Effect coverage: {most_common}")
        if self.warnings:
            print("Warnings:")
            for warning in self.warnings:
                print(f" - {warning}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate project JSON content and data-driven cross references."
    )
    parser.parse_args()
    return ProjectValidator().run()


if __name__ == "__main__":
    sys.exit(main())
