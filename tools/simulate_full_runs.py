#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import math
import random
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from pathlib import Path
from statistics import mean
from typing import Any

from simulate_balance import (
    card_utility,
    effect_amount,
    load_balance_targets,
    load_encounters,
    load_json,
    load_list,
    simulate_encounter,
    simulate_starter_deck,
)

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "data"
REPORTS = ROOT / "reports"
MAX_STRESS = 200

RARITY_RANK = {"starter": 0, "common": 1, "uncommon": 2, "rare": 3, "special": 4}
RELIC_POWER = {"common": 0.045, "uncommon": 0.06, "rare": 0.08, "boss": 0.10, "starter": 0.0, "special": 0.0}


def scaled_price(base: int, floor_index: int, percent: int) -> int:
    return max(0, round(max(0, base) * (1.0 + max(0, floor_index - 1) * max(0, percent) / 100.0)))


def value_amount(value: Any) -> int:
    if not isinstance(value, dict):
        return 0
    if value.get("type") == "fixed":
        amount = value.get("amount", 0)
        return int(amount) if isinstance(amount, int) and not isinstance(amount, bool) else 0
    return int(round(effect_amount({"value": value})))


def card_themes(card: dict[str, Any]) -> set[str]:
    result: set[str] = set()
    for effect in card.get("effects", []):
        if not isinstance(effect, dict):
            continue
        effect_type = str(effect.get("type", ""))
        if effect_type in {"damage", "lose_hp", "spend_stress_damage"}:
            result.add("damage")
            if int(effect.get("repeat_count", effect.get("repeat", 1)) or 1) > 1:
                result.add("multi_hit")
        elif effect_type in {"block", "spend_stress_block"}:
            result.add("block")
        elif effect_type == "heal":
            result.add("sustain")
        elif effect_type in {"draw_cards", "recover_cards", "spend_stress_draw"}:
            result.add("draw")
        elif effect_type == "discard_cards":
            result.add("discard")
        elif effect_type in {"gain_energy", "lose_energy", "spend_stress_energy"}:
            result.add("energy")
        elif "stress" in effect_type:
            result.add("stress")
        elif effect_type == "apply_status":
            result.add("status")
        elif effect_type == "enter_stance":
            result.add("stance")
        elif effect_type in {"summon_drone", "use_drone"}:
            result.add("drone")
    if "exhaust" in card.get("keywords", []):
        result.add("exhaust")
    return result


def reward_score(candidate: dict[str, Any], deck: list[str], cards: dict[str, dict[str, Any]]) -> float:
    candidate_themes = card_themes(candidate)
    deck_cards = [cards[card_id] for card_id in deck if card_id in cards]
    copies = deck.count(str(candidate.get("id", "")))
    score = {"common": 0.0, "uncommon": 2.0, "rare": 4.0}.get(str(candidate.get("rarity", "")), -20.0)
    score -= 14.0 * copies
    theme_counts = Counter(theme for card in deck_cards for theme in card_themes(card))
    for theme in candidate_themes:
        score += min(4, theme_counts[theme]) * 3.0
    deck_size = max(1, len(deck_cards))
    if "damage" in candidate_themes and theme_counts["damage"] * 3 < deck_size:
        score += 8.0
    if "block" in candidate_themes and theme_counts["block"] * 3 < deck_size:
        score += 8.0
    if "sustain" in candidate_themes and theme_counts["sustain"] == 0:
        score += 4.0
    if "drone" in candidate_themes and theme_counts["drone"] == 0 and any(
        effect.get("type") == "use_drone" for effect in candidate.get("effects", []) if isinstance(effect, dict)
    ):
        score -= 24.0
    return score


def stress_management(card: dict[str, Any]) -> float:
    result = float(max(0, int(card.get("stress_cost", 0) or 0)))
    for effect in card.get("effects", []):
        if not isinstance(effect, dict):
            continue
        effect_type = str(effect.get("type", ""))
        amount = float(max(0, value_amount(effect.get("value"))))
        if effect_type == "lose_stress":
            result += amount
        elif effect_type.startswith("spend_stress_"):
            result += amount * 0.7
    return result


def relic_gold_multiplier(relic_ids: list[str], relics: dict[str, dict[str, Any]]) -> float:
    result = 1.0
    for relic_id in relic_ids:
        relic = relics.get(relic_id, {})
        for modifier in relic.get("modifiers", []):
            if isinstance(modifier, dict) and modifier.get("type") == "gold_reward_multiply":
                result *= float(modifier.get("multiplier", 1.0) or 1.0)
    return result


def mechanic_allows_relic(relic: dict[str, Any], mechanic_id: str) -> bool:
    relic_mechanic = str(relic.get("mechanic_id", ""))
    return not relic_mechanic or relic_mechanic == "default" or relic_mechanic == mechanic_id


def event_requirement_met(requirements: Any, state: "RunState") -> bool:
    if not isinstance(requirements, dict):
        return True
    if state.gold < int(requirements.get("min_gold", 0) or 0):
        return False
    if state.hp < int(requirements.get("min_hp", 0) or 0):
        return False
    if len(state.deck) < int(requirements.get("min_deck_size", 0) or 0):
        return False
    max_deck = int(requirements.get("max_deck_size", 0) or 0)
    if max_deck > 0 and len(state.deck) > max_deck:
        return False
    if state.max_hp - state.hp < int(requirements.get("min_missing_hp", 0) or 0):
        return False
    if len(state.upgraded_indices) < int(requirements.get("min_upgraded_cards", 0) or 0):
        return False
    if state.stress < int(requirements.get("min_stress", 0) or 0):
        return False
    max_stress = int(requirements.get("max_stress", 0) or 0)
    if max_stress > 0 and state.stress > max_stress:
        return False
    mechanic = str(requirements.get("mechanic_id", ""))
    if mechanic and mechanic != state.mechanic_id:
        return False
    required_relics = requirements.get("has_relics", requirements.get("has_relic", []))
    if isinstance(required_relics, str):
        required_relics = [required_relics]
    if any(str(item) not in state.relics for item in required_relics or []):
        return False
    missing_relics = requirements.get("missing_relics", requirements.get("missing_relic", []))
    if isinstance(missing_relics, str):
        missing_relics = [missing_relics]
    if any(str(item) in state.relics for item in missing_relics or []):
        return False
    required_cards = requirements.get("has_cards", requirements.get("has_card", []))
    if isinstance(required_cards, str):
        required_cards = [required_cards]
    if any(str(item) not in state.deck for item in required_cards or []):
        return False
    missing_cards = requirements.get("missing_cards", requirements.get("missing_card", []))
    if isinstance(missing_cards, str):
        missing_cards = [missing_cards]
    if any(str(item) in state.deck for item in missing_cards or []):
        return False
    required_flags = requirements.get("has_flags", requirements.get("has_flag", []))
    if isinstance(required_flags, str):
        required_flags = [required_flags]
    if any(str(item) not in state.flags for item in required_flags or []):
        return False
    missing_flags = requirements.get("missing_flags", requirements.get("missing_flag", []))
    if isinstance(missing_flags, str):
        missing_flags = [missing_flags]
    if any(str(item) in state.flags for item in missing_flags or []):
        return False
    if bool(requirements.get("free_consumable_slot", requirements.get("requires_free_consumable_slot", False))):
        if state.consumables >= 3:
            return False
    return True


@dataclass
class RunState:
    archetype_id: str
    mechanic_id: str
    reward_pools: list[str]
    actor_ids: list[str]
    max_hp: float
    hp: float
    gold: int
    deck: list[str]
    relics: list[str]
    stress: float = 0.0
    upgraded_indices: set[int] = field(default_factory=set)
    consumables: int = 0
    has_active_item: bool = False
    build_power: float = 1.0
    cards_added: int = 0
    cards_skipped: int = 0
    cards_removed: int = 0
    relics_gained: int = 0
    shops_visited: int = 0
    shop_purchases: int = 0
    rests_healed: int = 0
    rests_calmed: int = 0
    rests_upgraded: int = 0
    events_seen: set[str] = field(default_factory=set)
    flags: set[str] = field(default_factory=set)
    room_count: int = 0
    combat_count: int = 0
    elite_count: int = 0
    event_count: int = 0
    death_floor: str = ""
    death_room_type: str = ""
    death_encounter: str = ""


@dataclass
class SimulationModel:
    cards: dict[str, dict[str, Any]]
    relics: dict[str, dict[str, Any]]
    actors: dict[str, dict[str, Any]]
    archetypes: list[dict[str, Any]]
    events: list[dict[str, Any]]
    floors: list[dict[str, Any]]
    maps: dict[str, dict[str, Any]]
    encounter_models: dict[tuple[str, str], list[dict[str, Any]]]
    reward_tuning: dict[str, Any]
    shop_tuning: dict[str, Any]
    balance_targets: dict[str, Any]


def choose_weighted(items: list[dict[str, Any]], rng: random.Random) -> dict[str, Any]:
    weights = [max(1, int(item.get("weight", 1) or 1)) for item in items]
    return rng.choices(items, weights=weights, k=1)[0]


def reward_candidates(state: RunState, model: SimulationModel, minimum_rarity: str | None = None) -> list[dict[str, Any]]:
    result: list[dict[str, Any]] = []
    minimum_rank = RARITY_RANK.get(minimum_rarity or "common", 1)
    for card in model.cards.values():
        rarity = str(card.get("rarity", ""))
        if rarity in {"starter", "special"} or RARITY_RANK.get(rarity, -1) < minimum_rank:
            continue
        pool = str(card.get("card_pool", card.get("reward_pool", card.get("owner_actor", ""))))
        if pool and pool in state.reward_pools:
            result.append(card)
    return result


def choose_card_offer(state: RunState, model: SimulationModel, rng: random.Random, count: int = 3, minimum_rarity: str | None = None) -> list[dict[str, Any]]:
    candidates = reward_candidates(state, model, minimum_rarity)
    result: list[dict[str, Any]] = []
    while candidates and len(result) < count:
        scored: list[tuple[float, dict[str, Any]]] = []
        for candidate in candidates:
            diversity_penalty = 5.0 * sum(len(card_themes(candidate) & card_themes(selected)) for selected in result)
            score = reward_score(candidate, state.deck, model.cards) - diversity_penalty
            if result:
                score += rng.uniform(0.0, 10.0)
            else:
                score *= 2.0
            scored.append((score, candidate))
        _, picked = max(scored, key=lambda item: item[0])
        result.append(picked)
        candidates.remove(picked)
    return result


def accept_best_card(state: RunState, model: SimulationModel, rng: random.Random, minimum_rarity: str | None = None, purchased: bool = False) -> dict[str, Any] | None:
    offers = choose_card_offer(state, model, rng, 3, minimum_rarity)
    if not offers:
        return None
    best = max(offers, key=lambda card: reward_score(card, state.deck, model.cards) + 0.25 * card_utility(card))
    utilities = [card_utility(model.cards[card_id]) for card_id in state.deck if card_id in model.cards]
    deck_average = mean(utilities) if utilities else 0.0
    threshold = max(5.0, deck_average * (0.80 + max(0, len(state.deck) - 12) * 0.012))
    if not purchased and card_utility(best) < threshold and rng.random() < 0.72:
        state.cards_skipped += 1
        return None
    state.deck.append(str(best["id"]))
    state.cards_added += 1
    relative = card_utility(best) / max(8.0, deck_average if deck_average > 0 else 8.0)
    state.build_power += min(0.055, 0.022 + max(0.0, relative - 0.8) * 0.018)
    return best


def add_random_relic(state: RunState, model: SimulationModel, rng: random.Random) -> str | None:
    candidates = [
        relic for relic in model.relics.values()
        if str(relic.get("rarity", "")) not in {"starter", "special"}
        and str(relic.get("id", "")) not in state.relics
        and mechanic_allows_relic(relic, state.mechanic_id)
    ]
    if not candidates:
        return None
    relic = rng.choice(candidates)
    relic_id = str(relic["id"])
    state.relics.append(relic_id)
    state.relics_gained += 1
    state.build_power += RELIC_POWER.get(str(relic.get("rarity", "common")), 0.05)
    return relic_id


def upgrade_best_card(state: RunState, model: SimulationModel) -> bool:
    candidates: list[tuple[float, int]] = []
    for index, card_id in enumerate(state.deck):
        if index in state.upgraded_indices:
            continue
        card = model.cards.get(card_id)
        if not card or not isinstance(card.get("upgrade"), dict):
            continue
        if str(card.get("type", "")) in {"status", "curse"}:
            continue
        candidates.append((card_utility(card), index))
    if not candidates:
        return False
    _, index = max(candidates)
    state.upgraded_indices.add(index)
    state.build_power += 0.045
    return True


def remove_worst_card(state: RunState, model: SimulationModel) -> bool:
    if len(state.deck) <= 8:
        return False
    candidates = [(card_utility(model.cards[card_id]), index) for index, card_id in enumerate(state.deck) if card_id in model.cards]
    if not candidates:
        return False
    _, index = min(candidates)
    state.deck.pop(index)
    state.upgraded_indices = {old - 1 if old > index else old for old in state.upgraded_indices if old != index}
    state.cards_removed += 1
    state.build_power += 0.035
    return True


def event_choice_value(choice: dict[str, Any], state: RunState) -> float:
    score = 0.0
    missing = max(0.0, state.max_hp - state.hp)
    for effect in choice.get("effects", []):
        if not isinstance(effect, dict):
            continue
        kind = str(effect.get("type", ""))
        amount = float(effect.get("amount", 0) or 0)
        if kind == "gain_gold": score += 0.08 * amount
        elif kind == "lose_gold": score -= 0.08 * amount
        elif kind in {"gain_relic", "gain_random_relic"}: score += 7.0
        elif kind in {"gain_card", "gain_random_card"}: score += 2.5
        elif kind == "upgrade_random_card": score += 5.0
        elif kind in {"remove_card", "remove_random_card"}: score += 4.0 if len(state.deck) > 10 else 1.0
        elif kind == "gain_random_consumable": score += 2.2 if state.consumables < 3 else 0.0
        elif kind == "heal_all": score += min(missing, amount) * 0.45
        elif kind == "lose_hp": score -= amount * (0.85 if state.hp < state.max_hp * 0.45 else 0.5)
        elif kind == "gain_stress": score += amount * (0.025 if state.mechanic_id == "stress_psychopath" else -0.10)
        elif kind == "lose_stress": score += min(state.stress, amount) * (0.08 if state.mechanic_id == "stress_psychopath" else 0.12)
        elif kind in {"set_flag", "clear_flag"}: score += 0.4
    return score


def apply_event_choice(choice: dict[str, Any], state: RunState, model: SimulationModel, rng: random.Random) -> None:
    for effect in choice.get("effects", []):
        if not isinstance(effect, dict):
            continue
        kind = str(effect.get("type", ""))
        amount = int(effect.get("amount", 0) or 0)
        content_id = str(effect.get("content_id", effect.get("id", effect.get("flag", effect.get("flag_id", "")))))
        if kind == "gain_gold": state.gold += amount
        elif kind == "lose_gold": state.gold = max(0, state.gold - amount)
        elif kind == "gain_random_relic": add_random_relic(state, model, rng)
        elif kind == "gain_relic" and content_id in model.relics and content_id not in state.relics:
            state.relics.append(content_id); state.relics_gained += 1; state.build_power += RELIC_POWER.get(str(model.relics[content_id].get("rarity", "common")), 0.05)
        elif kind == "gain_random_card": accept_best_card(state, model, rng)
        elif kind == "gain_card" and content_id in model.cards:
            state.deck.append(content_id); state.cards_added += 1; state.build_power += 0.03
        elif kind == "upgrade_random_card": upgrade_best_card(state, model)
        elif kind in {"remove_random_card", "remove_card"}: remove_worst_card(state, model)
        elif kind == "gain_random_consumable": state.consumables = min(3, state.consumables + 1)
        elif kind == "heal_all": state.hp = min(state.max_hp, state.hp + amount * max(1, len(state.actor_ids)))
        elif kind == "lose_hp": state.hp = max(1.0, state.hp - amount * max(1, len(state.actor_ids)))
        elif kind == "gain_stress": state.stress = min(MAX_STRESS, state.stress + amount)
        elif kind == "lose_stress": state.stress = max(0.0, state.stress - amount)
        elif kind == "set_flag" and content_id: state.flags.add(content_id)
        elif kind == "clear_flag" and content_id: state.flags.discard(content_id)


def simulate_event(state: RunState, floor_id: str, model: SimulationModel, rng: random.Random) -> None:
    pool = [
        event for event in model.events
        if floor_id.replace("floor", "act") in event.get("event_pools", [])
        and event_requirement_met(event.get("requirements", {}), state)
    ]
    unseen = [event for event in pool if str(event.get("id", "")) not in state.events_seen]
    candidates = unseen or pool
    if not candidates:
        return
    event = rng.choice(candidates)
    event_id = str(event.get("id", ""))
    if event_id:
        state.events_seen.add(event_id)
        state.flags.add("event.seen." + event_id)
    choices = [choice for choice in event.get("choices", []) if isinstance(choice, dict) and event_requirement_met(choice.get("requirements", {}), state)]
    if not choices:
        return
    choice = max(choices, key=lambda item: event_choice_value(item, state) + rng.uniform(0.0, 0.8))
    apply_event_choice(choice, state, model, rng)


def node_probabilities(config: dict[str, Any], layer: int) -> dict[str, float]:
    counts = config.get("layer_node_counts", [])
    width = int(counts[layer]) if 0 <= layer < len(counts) else 1
    result: dict[str, float] = defaultdict(float)
    for key, node_type in (("chests", "chest"), ("shop", "shop"), ("elites", "elite"), ("events", "event")):
        spec = config.get(key)
        if not isinstance(spec, dict):
            continue
        min_layer = int(spec.get("min_layer", 1))
        max_layer = int(spec.get("max_layer", len(counts) - 3))
        if not (min_layer <= layer <= max_layer):
            continue
        if key == "chests" and bool(spec.get("full_layer", False)) and min_layer == max_layer == layer:
            result[node_type] = 1.0
            continue
        if key in {"elites", "events"}:
            expected_count = 0.5 * (float(spec.get("min", 0)) + float(spec.get("max", spec.get("min", 0))))
        else:
            expected_count = float(spec.get("count", 0))
        eligible_slots = sum(int(counts[index]) for index in range(max(0, min_layer), min(len(counts), max_layer + 1)))
        if eligible_slots > 0:
            result[node_type] += expected_count / float(eligible_slots)
    special_total = sum(result.values())
    if special_total >= 0.92:
        scale = 0.92 / special_total
        for key in list(result):
            result[key] *= scale
        special_total = sum(result.values())
    remaining = max(0.0, 1.0 - special_total)
    if isinstance(config.get("events"), dict):
        result["combat"] += remaining
    else:
        combat_weight = max(0, int(config.get("combat_weight", 70)))
        event_weight = max(0, int(config.get("event_weight", 30)))
        total = max(1, combat_weight + event_weight)
        result["combat"] += remaining * combat_weight / total
        result["event"] += remaining * event_weight / total
    return dict(result)


def choose_room_type(config: dict[str, Any], layer: int, rng: random.Random) -> str:
    layers = config.get("layer_node_counts", [])
    if layer == 0:
        return "combat"
    if layer == len(layers) - 2:
        return "rest"
    if layer == len(layers) - 1:
        return "boss"
    probabilities = node_probabilities(config, layer)
    roll = rng.random()
    cumulative = 0.0
    for node_type in ("chest", "shop", "elite", "event", "combat"):
        cumulative += probabilities.get(node_type, 0.0)
        if roll <= cumulative:
            return node_type
    return "combat"


def choose_encounter(floor_id: str, pool: str, layer: int, model: SimulationModel, rng: random.Random) -> dict[str, Any] | None:
    entries = model.encounter_models.get((floor_id, pool), [])
    eligible = [entry for entry in entries if int(entry.get("min_layer", -1)) <= layer <= int(entry.get("max_layer", 10**9))]
    if not eligible:
        eligible = entries
    return choose_weighted(eligible, rng) if eligible else None


def combat_stress_sink(state: RunState, model: SimulationModel) -> float:
    values = [stress_management(model.cards[card_id]) for card_id in state.deck if card_id in model.cards]
    if not values:
        return 0.0
    # Only a fraction of stress-management cards are drawn and worth playing each combat.
    return min(24.0, sum(values) / max(6.0, len(state.deck)) * 2.4)


def consume_emergency_item(state: RunState) -> None:
    if state.consumables <= 0:
        return
    if state.hp < state.max_hp * 0.42:
        state.consumables -= 1
        state.hp = min(state.max_hp, state.hp + state.max_hp * 0.13)
    elif state.stress > 150:
        state.consumables -= 1
        state.stress = max(0.0, state.stress - 24.0)


def apply_combat(state: RunState, floor_index: int, encounter: dict[str, Any], model: SimulationModel, rng: random.Random, attrition_scale: float) -> bool:
    floor_id = f"floor{floor_index}"
    expected_power = {1: 1.06, 2: 1.32, 3: 1.62, 4: 1.92, 5: 2.18}.get(floor_index, 1.0)
    psychopath_bonus = 1.0
    if state.mechanic_id == "stress_psychopath":
        psychopath_bonus += min(0.20, max(0.0, state.stress - 80.0) / 600.0)
    actual_power = max(0.55, state.build_power * psychopath_bonus)
    power_ratio = expected_power / actual_power
    base_loss = float(encounter.get("estimated_hp_loss", 0.0) or 0.0)
    hp_loss = base_loss * math.pow(max(0.55, power_ratio), 1.18) * rng.uniform(0.78, 1.25) * attrition_scale
    if state.has_active_item:
        hp_loss *= 0.96
    if state.consumables > 0 and hp_loss > state.hp * 0.22:
        state.consumables -= 1
        hp_loss *= 0.72
    hp_loss = max(0.0, hp_loss)
    state.hp -= hp_loss

    explicit_stress = float(encounter.get("estimated_stress", 0.0) or 0.0) * rng.uniform(0.8, 1.2)
    damage_stress = min(14.0, hp_loss / 5.0)
    gained_stress = max(0.0, explicit_stress + damage_stress - combat_stress_sink(state, model))
    state.stress = min(MAX_STRESS, state.stress + gained_stress)
    consume_emergency_item(state)

    if state.hp <= 0.0:
        state.death_floor = floor_id
        state.death_room_type = str(encounter.get("pool", "combat"))
        state.death_encounter = str(encounter.get("encounter_id", ""))
        return False
    if state.stress >= MAX_STRESS:
        state.death_floor = floor_id
        state.death_room_type = "stress_collapse"
        state.death_encounter = str(encounter.get("encounter_id", ""))
        return False
    return True


def award_combat(state: RunState, floor_index: int, room_type: str, encounter: dict[str, Any], model: SimulationModel, rng: random.Random) -> None:
    tuning = model.reward_tuning
    node = tuning.get("nodes", {}).get(room_type, {})
    base_gold = int(node.get("gold", 0) or 0)
    gold = base_gold * (1.0 + max(0, floor_index - 1) * int(tuning.get("gold_growth_percent_per_floor", 0) or 0) / 100.0)
    enemy_count = int(encounter.get("enemy_count", 1) or 1)
    gold *= 1.0 + max(0, enemy_count - 1) * int(tuning.get("group_gold_bonus_percent_per_extra_enemy", 0) or 0) / 100.0
    gold *= relic_gold_multiplier(state.relics, model.relics)
    if state.mechanic_id == "merchant_progression":
        gold *= float(tuning.get("merchant_gold_multiplier", 1.0) or 1.0)
    state.gold += int(gold)

    if bool(node.get("offer_cards", False)) and state.mechanic_id != "merchant_progression":
        accept_best_card(state, model, rng, str(node.get("minimum_card_rarity", "")) or None)
    if bool(node.get("guaranteed_relic", False)):
        add_random_relic(state, model, rng)
    if rng.random() < int(node.get("consumable_chance_percent", 0) or 0) / 100.0:
        state.consumables = min(3, state.consumables + 1)
    if rng.random() < int(node.get("active_item_chance_percent", 0) or 0) / 100.0:
        state.has_active_item = True


def visit_chest(state: RunState, model: SimulationModel, rng: random.Random) -> None:
    chance = int(model.reward_tuning.get("nodes", {}).get("chest", {}).get("active_item_chance_percent", 0) or 0)
    if rng.random() < chance / 100.0:
        state.has_active_item = True
    else:
        add_random_relic(state, model, rng)


def visit_rest(state: RunState, model: SimulationModel, rng: random.Random, floor_index: int) -> None:
    if state.mechanic_id == "merchant_progression":
        for _ in range(2):
            offers = choose_card_offer(state, model, rng, 5)
            if not offers:
                break
            best = max(offers, key=lambda card: reward_score(card, state.deck, model.cards) + 0.25 * card_utility(card))
            price = max(int(model.shop_tuning.get("minimum_card_price", 20)), int(scaled_price(int(best.get("gold_cost", 0)), floor_index, int(model.shop_tuning.get("card_price_growth_percent_per_floor", 4))) * float(model.shop_tuning.get("merchant_rest_card_price_multiplier", 0.85))))
            if state.gold >= price:
                state.gold -= price
                state.deck.append(str(best["id"])); state.cards_added += 1; state.build_power += 0.035
        return
    if state.stress > (165 if state.mechanic_id == "stress_psychopath" else 125):
        state.stress = max(0.0, state.stress - 60.0)
        state.rests_calmed += 1
    elif state.hp < state.max_hp * 0.68:
        state.hp = min(state.max_hp, state.hp + state.max_hp * 0.30)
        state.rests_healed += 1
    elif upgrade_best_card(state, model):
        state.rests_upgraded += 1
    else:
        state.hp = min(state.max_hp, state.hp + state.max_hp * 0.30)
        state.rests_healed += 1


def visit_shop(state: RunState, model: SimulationModel, rng: random.Random, floor_index: int) -> None:
    state.shops_visited += 1
    tuning = model.shop_tuning
    purchases = 0
    relic_candidates = [r for r in model.relics.values() if str(r.get("rarity", "")) not in {"starter", "special"} and str(r.get("id", "")) not in state.relics and mechanic_allows_relic(r, state.mechanic_id)]
    if relic_candidates:
        relic = max(rng.sample(relic_candidates, min(2, len(relic_candidates))), key=lambda item: RELIC_POWER.get(str(item.get("rarity", "common")), 0.05))
        rarity = str(relic.get("rarity", "common"))
        base = int(tuning.get("relic_prices", {}).get(rarity, 175) or 175)
        price = scaled_price(base, floor_index, int(tuning.get("relic_price_growth_percent_per_floor", 5)))
        if state.gold >= price + 25 and rng.random() < 0.58:
            state.gold -= price
            state.relics.append(str(relic["id"])); state.relics_gained += 1
            state.build_power += RELIC_POWER.get(rarity, 0.05)
            purchases += 1
    offers = choose_card_offer(state, model, rng, int(tuning.get("card_offers", 3) or 3))
    if offers:
        best = max(offers, key=lambda card: reward_score(card, state.deck, model.cards) + 0.25 * card_utility(card))
        price = scaled_price(int(best.get("gold_cost", 0) or 0), floor_index, int(tuning.get("card_price_growth_percent_per_floor", 4)))
        if state.gold >= price and (state.mechanic_id == "merchant_progression" or rng.random() < 0.45):
            state.gold -= price
            state.deck.append(str(best["id"])); state.cards_added += 1; state.build_power += 0.035
            purchases += 1
    removal_price = int(tuning.get("card_removal_price", 75) or 75) + max(0, floor_index - 1) * int(tuning.get("card_removal_price_per_floor", 10) or 10) + state.cards_removed * int(tuning.get("card_removal_price_per_use", 25) or 25)
    if len(state.deck) > 13 and state.gold >= removal_price and rng.random() < 0.50:
        state.gold -= removal_price
        if remove_worst_card(state, model):
            purchases += 1
    state.shop_purchases += purchases


def init_state(archetype: dict[str, Any], model: SimulationModel) -> RunState:
    actor_ids = [str(item) for item in archetype.get("actors", [])]
    max_hp = sum(float(model.actors.get(actor_id, {}).get("max_hp", 1)) for actor_id in actor_ids)
    state = RunState(
        archetype_id=str(archetype["id"]),
        mechanic_id=str(archetype.get("mechanic_id", "")),
        reward_pools=[str(item) for item in archetype.get("reward_card_pools", [])],
        actor_ids=actor_ids,
        max_hp=max_hp,
        hp=max_hp,
        gold=int(archetype.get("starting_gold", 0) or 0),
        deck=[str(item) for item in archetype.get("starting_deck", [])],
        relics=[str(item) for item in archetype.get("starting_relics", [])],
    )
    return state


def run_one(archetype: dict[str, Any], model: SimulationModel, seed: int, attrition_scale: float) -> dict[str, Any]:
    rng = random.Random(seed)
    state = init_state(archetype, model)
    floor_snapshots: list[dict[str, Any]] = []
    for floor_def in model.floors:
        floor_id = str(floor_def["id"])
        floor_index = int(floor_def["index"])
        config = model.maps[str(floor_def["map_config_id"])]
        layers = config.get("layer_node_counts", [])
        for layer in range(len(layers)):
            room_type = choose_room_type(config, layer, rng)
            state.room_count += 1
            if room_type in {"combat", "elite", "boss"}:
                pool = room_type
                encounter = choose_encounter(floor_id, pool, layer, model, rng)
                if encounter is None:
                    continue
                if room_type == "combat": state.combat_count += 1
                elif room_type == "elite": state.elite_count += 1
                if not apply_combat(state, floor_index, encounter, model, rng, attrition_scale):
                    return finish_result(state, False, floor_snapshots)
                award_combat(state, floor_index, room_type, encounter, model, rng)
            elif room_type == "event":
                state.event_count += 1
                if rng.random() < int(config.get("question_mark_combat_chance", 0) or 0) / 100.0:
                    encounter = choose_encounter(floor_id, "combat", layer, model, rng)
                    if encounter is not None:
                        state.combat_count += 1
                        if not apply_combat(state, floor_index, encounter, model, rng, attrition_scale):
                            return finish_result(state, False, floor_snapshots)
                        award_combat(state, floor_index, "combat", encounter, model, rng)
                else:
                    simulate_event(state, floor_id, model, rng)
            elif room_type == "shop":
                visit_shop(state, model, rng, floor_index)
            elif room_type == "chest":
                visit_chest(state, model, rng)
            elif room_type == "rest":
                visit_rest(state, model, rng, floor_index)
            if state.stress >= MAX_STRESS:
                state.death_floor = floor_id; state.death_room_type = "stress_collapse"
                return finish_result(state, False, floor_snapshots)
        floor_snapshots.append({
            "floor_id": floor_id,
            "hp": round(state.hp, 3),
            "stress": round(state.stress, 3),
            "gold": state.gold,
            "deck_size": len(state.deck),
            "relic_count": len(state.relics),
            "build_power": round(state.build_power, 3),
        })
    return finish_result(state, True, floor_snapshots)


def finish_result(state: RunState, won: bool, floor_snapshots: list[dict[str, Any]]) -> dict[str, Any]:
    return {
        "archetype_id": state.archetype_id,
        "won": won,
        "death_floor": state.death_floor,
        "death_room_type": state.death_room_type,
        "death_encounter": state.death_encounter,
        "hp": round(max(0.0, state.hp), 3),
        "max_hp": round(state.max_hp, 3),
        "stress": round(state.stress, 3),
        "gold": state.gold,
        "deck_size": len(state.deck),
        "cards_added": state.cards_added,
        "cards_skipped": state.cards_skipped,
        "cards_removed": state.cards_removed,
        "upgraded_cards": len(state.upgraded_indices),
        "relic_count": len(state.relics),
        "relics_gained": state.relics_gained,
        "consumables": state.consumables,
        "active_item": state.has_active_item,
        "build_power": round(state.build_power, 3),
        "rooms": state.room_count,
        "combats": state.combat_count,
        "elites": state.elite_count,
        "events": state.event_count,
        "shops": state.shops_visited,
        "shop_purchases": state.shop_purchases,
        "rest_heals": state.rests_healed,
        "rest_calms": state.rests_calmed,
        "rest_upgrades": state.rests_upgraded,
        "floors": floor_snapshots,
    }


def build_model(encounter_samples: int, turns: int, seed: int) -> SimulationModel:
    cards = {str(item["id"]): item for item in load_list("cards") if isinstance(item.get("id"), str)}
    relics = {str(item["id"]): item for item in load_list("relics") if isinstance(item.get("id"), str)}
    enemies = {str(item["id"]): item for item in load_list("enemies") if isinstance(item.get("id"), str)}
    drones = {str(item["id"]): item for item in load_list("drones") if isinstance(item.get("id"), str)}
    actors_root = load_json(DATA / "actors" / "player_actors.json")
    actors = {str(item["id"]): item for item in actors_root if isinstance(item, dict)}
    archetypes = [item for item in load_json(DATA / "archetypes" / "playable_archetypes.json") if isinstance(item, dict) and item.get("is_available", True)]
    events = [item for item in load_json(DATA / "events" / "run_events.json") if isinstance(item, dict)]
    floors_root = load_json(DATA / "run" / "floors.json")
    floors = [item for item in floors_root.get("floors", []) if isinstance(item, dict) and item.get("is_implemented", False)]
    maps = {path.stem: load_json(path) for path in sorted((DATA / "run" / "acts").glob("*.json"))}
    balance_targets = load_balance_targets()

    starter_reports: list[dict[str, Any]] = []
    for index, archetype in enumerate(archetypes):
        metrics = simulate_starter_deck(archetype, cards, relics, drones, max(20, encounter_samples), turns, seed + index * 10007)
        if len(archetype.get("actors", [])) == 1:
            starter_reports.append(metrics)
    average_damage = mean(item["damage_per_turn"] for item in starter_reports)
    average_block = mean(item["block_per_turn"] for item in starter_reports)

    encounter_models: dict[tuple[str, str], list[dict[str, Any]]] = defaultdict(list)
    floor_configs = balance_targets.get("floors", {})
    for index, (floor_id, pool, encounter) in enumerate(load_encounters()):
        metrics = simulate_encounter(encounter, enemies, encounter_samples, turns, seed + index * 7919)
        floor_config = floor_configs.get(floor_id, {})
        offense_multiplier = float(floor_config.get("offense_multiplier", 1.0))
        defense_multiplier = float(floor_config.get("defense_multiplier", 1.0))
        energy_efficiency = max(0.65, 1.0 - 0.12 * metrics["energy_loss_per_turn"])
        projected_damage = average_damage * offense_multiplier * energy_efficiency
        projected_damage *= max(0.5, 1.0 - metrics["player_offense_penalty"])
        projected_block = average_block * defense_multiplier * energy_efficiency
        defensive_drag = 0.55 * metrics["enemy_block_per_turn"] + 0.7 * metrics["enemy_heal_per_turn"]
        effective_player_damage = max(projected_damage * 0.55, projected_damage - defensive_drag)
        turns_to_kill = metrics["total_hp"] / max(0.1, effective_player_damage)
        hp_loss = max(0.0, metrics["incoming_damage_per_turn"] - projected_block) * turns_to_kill
        encounter_models[(floor_id, pool)].append({
            "encounter_id": str(encounter.get("id", "")),
            "pool": pool,
            "weight": int(encounter.get("weight", 1) or 1),
            "min_layer": int(encounter.get("min_layer", -1) or -1),
            "max_layer": int(encounter.get("max_layer", 10**9) or 10**9),
            "enemy_count": len(encounter.get("enemies", [])),
            "estimated_hp_loss": hp_loss,
            "estimated_stress": metrics["stress_per_turn"] * turns_to_kill,
        })

    return SimulationModel(
        cards=cards,
        relics=relics,
        actors=actors,
        archetypes=archetypes,
        events=events,
        floors=floors,
        maps=maps,
        encounter_models=dict(encounter_models),
        reward_tuning=load_json(DATA / "rewards" / "reward_tables.json"),
        shop_tuning=load_json(DATA / "shop" / "shop_tables.json"),
        balance_targets=balance_targets,
    )


def summarize(results: list[dict[str, Any]], runs_per_archetype: int, seed: int, encounter_samples: int) -> dict[str, Any]:
    groups: list[dict[str, Any]] = []
    for archetype_id in sorted({str(item["archetype_id"]) for item in results}):
        items = [item for item in results if item["archetype_id"] == archetype_id]
        wins = [item for item in items if item["won"]]
        deaths = Counter(str(item["death_floor"] or "victory") for item in items)
        death_types = Counter(str(item["death_room_type"] or "victory") for item in items)
        death_encounters = Counter(str(item["death_encounter"]) for item in items if item["death_encounter"])
        groups.append({
            "archetype_id": archetype_id,
            "runs": len(items),
            "wins": len(wins),
            "win_rate": round(len(wins) / max(1, len(items)), 4),
            "average_rooms": round(mean(float(item["rooms"]) for item in items), 3),
            "average_final_hp_percent": round(mean(100.0 * float(item["hp"]) / max(1.0, float(item["max_hp"])) for item in items), 3),
            "average_final_stress": round(mean(float(item["stress"]) for item in items), 3),
            "average_gold": round(mean(float(item["gold"]) for item in items), 3),
            "average_deck_size": round(mean(float(item["deck_size"]) for item in items), 3),
            "average_cards_added": round(mean(float(item["cards_added"]) for item in items), 3),
            "average_cards_removed": round(mean(float(item["cards_removed"]) for item in items), 3),
            "average_upgraded_cards": round(mean(float(item["upgraded_cards"]) for item in items), 3),
            "average_relic_count": round(mean(float(item["relic_count"]) for item in items), 3),
            "average_build_power": round(mean(float(item["build_power"]) for item in items), 3),
            "average_shop_purchases": round(mean(float(item["shop_purchases"]) for item in items), 3),
            "death_floors": dict(sorted(deaths.items())),
            "death_room_types": dict(sorted(death_types.items())),
            "top_death_encounters": [
                {"encounter_id": encounter_id, "deaths": count}
                for encounter_id, count in death_encounters.most_common(8)
            ],
        })

    floor_survival: list[dict[str, Any]] = []
    for floor_index in range(1, 6):
        floor_id = f"floor{floor_index}"
        reached = sum(1 for item in results if item["won"] or not item["death_floor"] or int(str(item["death_floor"]).replace("floor", "") or 99) >= floor_index)
        died = sum(1 for item in results if item["death_floor"] == floor_id)
        floor_survival.append({"floor_id": floor_id, "reached": reached, "deaths": died, "death_rate_among_reached": round(died / max(1, reached), 4)})

    floor_progression: list[dict[str, Any]] = []
    for floor_index in range(1, 6):
        floor_id = f"floor{floor_index}"
        snapshots = [
            snapshot
            for item in results
            for snapshot in item.get("floors", [])
            if snapshot.get("floor_id") == floor_id
        ]
        floor_progression.append({
            "floor_id": floor_id,
            "completed_samples": len(snapshots),
            "average_hp": round(mean(float(item["hp"]) for item in snapshots), 3) if snapshots else 0.0,
            "average_stress": round(mean(float(item["stress"]) for item in snapshots), 3) if snapshots else 0.0,
            "average_gold": round(mean(float(item["gold"]) for item in snapshots), 3) if snapshots else 0.0,
            "average_deck_size": round(mean(float(item["deck_size"]) for item in snapshots), 3) if snapshots else 0.0,
            "average_relic_count": round(mean(float(item["relic_count"]) for item in snapshots), 3) if snapshots else 0.0,
            "average_build_power": round(mean(float(item["build_power"]) for item in snapshots), 3) if snapshots else 0.0,
        })

    overall_death_encounters = Counter(str(item["death_encounter"]) for item in results if item["death_encounter"])
    return {
        "schema_version": 1,
        "model": "heuristic_full_run_monte_carlo",
        "limitations": [
            "Combat uses the existing encounter Monte Carlo model rather than playing cards turn-by-turn.",
            "Map routing samples room types from generated-map placement probabilities instead of reproducing graph connectivity.",
            "Party HP and stress are aggregated; two-actor targeting and individual deaths are approximated.",
            "Archetype-specific combat engines (especially Merchant economy, drones, stances, and stress resolve) are represented only through coarse build-power heuristics.",
        ],
        "runs_per_archetype": runs_per_archetype,
        "total_runs": len(results),
        "seed": seed,
        "encounter_samples": encounter_samples,
        "overall_win_rate": round(sum(1 for item in results if item["won"]) / max(1, len(results)), 4),
        "archetypes": groups,
        "floor_survival": floor_survival,
        "floor_progression": floor_progression,
        "top_death_encounters": [
            {"encounter_id": encounter_id, "deaths": count}
            for encounter_id, count in overall_death_encounters.most_common(15)
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Approximate complete five-floor run simulation")
    parser.add_argument("--runs", type=int, default=1000, help="Runs per playable archetype")
    parser.add_argument("--encounter-samples", type=int, default=40, help="Monte Carlo samples used to build each encounter pressure model")
    parser.add_argument("--turns", type=int, default=5)
    parser.add_argument("--seed", type=int, default=3617)
    parser.add_argument("--attrition-scale", type=float, default=0.42, help="Fraction of static encounter HP loss realized across a full run; use sensitivity runs to calibrate against telemetry")
    parser.add_argument("--output", type=Path, default=REPORTS / "full_run_simulation.json")
    parser.add_argument("--csv", type=Path, default=REPORTS / "full_run_simulation.csv")
    args = parser.parse_args()
    if args.runs <= 0 or args.encounter_samples <= 0 or args.turns <= 0 or args.attrition_scale <= 0:
        raise SystemExit("--runs, --encounter-samples, --turns and --attrition-scale must be positive")

    model = build_model(args.encounter_samples, args.turns, args.seed)
    results: list[dict[str, Any]] = []
    for archetype_index, archetype in enumerate(model.archetypes):
        for run_index in range(args.runs):
            run_seed = args.seed + archetype_index * 10_000_019 + run_index * 1009
            results.append(run_one(archetype, model, run_seed, args.attrition_scale))

    summary = summarize(results, args.runs, args.seed, args.encounter_samples)
    summary["attrition_scale"] = args.attrition_scale
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    fields = [
        "archetype_id", "won", "death_floor", "death_room_type", "death_encounter", "hp", "max_hp", "stress", "gold",
        "deck_size", "cards_added", "cards_skipped", "cards_removed", "upgraded_cards", "relic_count", "relics_gained",
        "build_power", "rooms", "combats", "elites", "events", "shops", "shop_purchases", "rest_heals", "rest_calms", "rest_upgrades",
    ]
    args.csv.parent.mkdir(parents=True, exist_ok=True)
    with args.csv.open("w", encoding="utf-8", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=fields)
        writer.writeheader()
        for result in results:
            writer.writerow({key: result.get(key) for key in fields})

    print(f"Simulated {len(results)} complete run(s)")
    print(f"Overall win rate: {summary['overall_win_rate']:.1%}")
    for group in summary["archetypes"]:
        print(
            f"  {group['archetype_id']:<20} win={group['win_rate']:.1%} "
            f"rooms={group['average_rooms']:.1f} deck={group['average_deck_size']:.1f} "
            f"relics={group['average_relic_count']:.1f}"
        )
    print(f"Wrote {args.output}")
    print(f"Wrote {args.csv}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
