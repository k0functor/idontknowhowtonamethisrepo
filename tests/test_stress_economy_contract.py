#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(f"stress economy contract failed: {message}")


def load(path: str):
    return json.loads((ROOT / path).read_text(encoding="utf-8"))


rules = (ROOT / "src/run/StressEconomyRules.hpp").read_text(encoding="utf-8")
for token in ("DamagePerStress = 3", "MaximumStressFromSingleHit = 12", "RestCalmAmount = 60", "stressFromHpDamage"):
    if token not in rules:
        fail(f"missing stress economy rule: {token}")

source = (ROOT / "src/combat/EffectSystem.cpp").read_text(encoding="utf-8")
for token in ("addStressFromHpDamage", "damage.hpDamage", "state.isEnemy(source)", "state.isPlayer(target)"):
    if token not in source:
        fail(f"damage-to-stress integration is missing {token}")

cards = load("data/cards/lost_psychopath_cards.json")
by_id = {card["id"]: card for card in cards}
conversion_expectations = {
    "lost_psychopath_clean_cut": "spend_stress_damage",
    "lost_psychopath_low_guard": "spend_stress_block",
    "lost_psychopath_opening_gambit": "spend_stress_energy",
    "lost_psychopath_quick_thought": "spend_stress_draw",
}
for card_id, effect_type in conversion_expectations.items():
    card = by_id.get(card_id)
    if card is None:
        fail(f"missing conversion card {card_id}")
    effect = next((item for item in card.get("effects", []) if item.get("type") == effect_type), None)
    if effect is None:
        fail(f"{card_id} must use {effect_type}")
    if effect.get("value", {}).get("type") != "fixed" or effect.get("value", {}).get("amount", 0) <= 0:
        fail(f"{card_id} must have a positive fixed stress cost")
    if effect.get("output", 0) <= 0:
        fail(f"{card_id} must have a positive conversion output")

stress_generators = 0
stress_reducers = 0
for card in cards:
    for effect in card.get("effects", []):
        stress_generators += effect.get("type") == "gain_stress"
        stress_reducers += effect.get("type") == "lose_stress"
if stress_generators < 5:
    fail("the psychopath deck needs at least five stress-generating cards")
if stress_reducers < 5:
    fail("the psychopath deck needs at least five stress-reducing cards")

enemy_files = (
    "normal_enemies.json",
    "elite_enemies.json",
    "boss_enemies.json",
    "act2_enemies.json",
    "act3_enemies.json",
    "act5_enemies.json",
)
stress_actions = 0
for filename in enemy_files:
    for enemy in load(f"data/enemies/{filename}"):
        for action in enemy.get("actions", []):
            if any(
                effect.get("type") == "gain_stress" and effect.get("target") == "all_allies"
                for effect in action.get("effects", [])
            ):
                stress_actions += 1
if stress_actions < 15:
    fail(f"expected at least 15 enemy stress actions, found {stress_actions}")

run_controller = (ROOT / "src/run/RunController.cpp").read_text(encoding="utf-8")
map_scene = (ROOT / "src/scenes/RunMapScene.cpp").read_text(encoding="utf-8")
merchant_scene = (ROOT / "src/scenes/MerchantRestScene.cpp").read_text(encoding="utf-8")
for token, owner in (
    ("completeRestCalm", run_controller),
    ("RestCalmAmount", run_controller),
    ("restCalmButtonBounds", map_scene),
    ("rest.calm_preview", map_scene),
    ("merchant_rest.calm", merchant_scene),
):
    if token not in owner:
        fail(f"rest calming integration is missing {token}")

for locale in ("en", "ru"):
    core = load(f"data/localization/{locale}/core.json")
    cards_loc = load(f"data/localization/{locale}/cards.json")
    run = load(f"data/localization/{locale}/run.json")
    for key in ("ui.card_unplayable.not_enough_stress_actor",):
        if not core.get(key):
            fail(f"{locale}: missing {key}")
    for key in (
        "inspect.effect.spend_stress_damage",
        "inspect.effect.spend_stress_block",
        "inspect.effect.spend_stress_energy",
        "inspect.effect.spend_stress_draw",
        "inspect.effect.stress_cost",
    ):
        if not cards_loc.get(key):
            fail(f"{locale}: missing {key}")
    for key in ("rest.calm", "rest.calm_preview", "merchant_rest.calm", "run_defeat.rest_calms_label"):
        if not run.get(key):
            fail(f"{locale}: missing {key}")

print(f"Stress economy contract passed ({stress_actions} enemy stress actions)")
