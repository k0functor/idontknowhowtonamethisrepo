#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import math
import random
from collections import defaultdict
from pathlib import Path
from statistics import mean
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "data"
REPORTS = ROOT / "reports"
BALANCE_TARGETS = DATA / "balance" / "progression_targets.json"


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8-sig"))



def load_balance_targets() -> dict[str, Any]:
    root = load_json(BALANCE_TARGETS)
    if not isinstance(root, dict) or root.get("schema_version") != 1:
        raise ValueError("data/balance/progression_targets.json has an unsupported schema")
    return root


def weighted_mean(values: list[tuple[float, int]]) -> float:
    total_weight = sum(max(1, weight) for _, weight in values)
    if total_weight <= 0:
        return 0.0
    return sum(value * max(1, weight) for value, weight in values) / total_weight


def target_range(floor_config: dict[str, Any], pool: str) -> tuple[float, float]:
    raw = floor_config.get(f"{pool}_hp_loss_target", [0.0, float("inf")])
    if not isinstance(raw, list) or len(raw) != 2:
        return 0.0, float("inf")
    return float(raw[0]), float(raw[1])


def target_status(value: float, target: tuple[float, float]) -> str:
    if value < target[0]:
        return "below"
    if value > target[1]:
        return "above"
    return "within"

def load_list(directory: str) -> list[dict[str, Any]]:
    result: list[dict[str, Any]] = []
    for path in sorted((DATA / directory).glob("*.json")):
        root = load_json(path)
        if isinstance(root, list):
            result.extend(item for item in root if isinstance(item, dict))
    return result


def value_expected(value: Any) -> float:
    if not isinstance(value, dict):
        return 0.0
    kind = value.get("type")
    if kind == "fixed":
        amount = value.get("amount", 0)
        return float(amount) if isinstance(amount, int) and not isinstance(amount, bool) else 0.0
    if kind == "dice":
        count = value.get("count", 0)
        sides = value.get("die", value.get("sides"))
        bonus = value.get("bonus", 0)
        if isinstance(sides, str) and sides.startswith("d"):
            try:
                side_count = int(sides[1:])
            except ValueError:
                side_count = 0
        elif isinstance(sides, int):
            side_count = sides
        else:
            side_count = 0
        if all(isinstance(number, int) and not isinstance(number, bool) for number in (count, bonus)):
            return float(count) * (side_count + 1.0) / 2.0 + float(bonus)
    return 0.0


def effect_repeat(effect: dict[str, Any]) -> int:
    repeat = effect.get("repeat", 1)
    return repeat if isinstance(repeat, int) and not isinstance(repeat, bool) and repeat > 0 else 1


def estimated_scaling_bonus(effect: dict[str, Any]) -> float:
    scaling = effect.get("scaling")
    if not isinstance(scaling, dict):
        return 0.0

    # Static simulations do not know the exact combat state. These values model
    # a modest, reachable synergy state rather than assuming every condition is perfect.
    bonus = 0.0
    if isinstance(scaling.get("status"), str):
        bonus += float(scaling.get("bonus_if_status_present", 0)) * 0.55
        bonus += float(scaling.get("bonus_per_status_stack", 0)) * 2.0
    bonus += float(scaling.get("bonus_per_card_in_hand", 0)) * 4.0
    bonus += float(scaling.get("bonus_per_card_in_discard", 0)) * 4.0
    maximum = scaling.get("maximum_bonus", -1)
    if isinstance(maximum, int) and not isinstance(maximum, bool) and maximum >= 0:
        bonus = min(bonus, float(maximum))
    return max(0.0, bonus)


def effect_metrics(effect: dict[str, Any]) -> dict[str, float]:
    repeat = effect_repeat(effect)
    amount = (value_expected(effect.get("value")) + estimated_scaling_bonus(effect)) * repeat
    effect_type = effect.get("type")
    metrics: dict[str, float] = defaultdict(float)
    if effect_type in {"damage", "spend_stress_damage"}:
        target = effect.get("target")
        multiplier = 1.6 if target == "all_enemies" else 1.0
        metrics["damage"] += amount * multiplier
    elif effect_type in {"block", "spend_stress_block"}:
        target = effect.get("target")
        multiplier = 1.5 if target in {"all_allies", "all_players"} else 1.0
        metrics["block"] += amount * multiplier
    elif effect_type == "apply_status":
        status = effect.get("status")
        if status == "poison":
            metrics["damage"] += amount * 2.0
        elif status in {"strength", "onslaught"}:
            metrics["scaling"] += amount * 2.5
        elif status == "dexterity":
            metrics["scaling"] += amount * 2.0
        elif status in {"weak", "vulnerable"}:
            metrics["control"] += amount * 2.0
    elif effect_type in {"draw", "draw_cards", "spend_stress_draw"}:
        metrics["draw"] += amount
    elif effect_type == "recover_cards":
        metrics["draw"] += amount * 0.8
    elif effect_type in {"gain_energy", "spend_stress_energy"}:
        metrics["energy"] += amount
    elif effect_type == "gain_stress":
        metrics["stress"] += amount
    elif effect_type == "lose_energy":
        metrics["energy_loss"] += amount
    elif effect_type == "heal":
        metrics["heal"] += amount
    return dict(metrics)


def card_metrics(card: dict[str, Any]) -> dict[str, float]:
    result: dict[str, float] = defaultdict(float)
    for effect in card.get("effects", []):
        if isinstance(effect, dict):
            for key, value in effect_metrics(effect).items():
                result[key] += value
    return dict(result)


def card_utility(card: dict[str, Any]) -> float:
    metrics = card_metrics(card)
    return (
        metrics.get("damage", 0.0)
        + 0.9 * metrics.get("block", 0.0)
        + metrics.get("scaling", 0.0)
        + metrics.get("control", 0.0)
        + 2.0 * metrics.get("draw", 0.0)
        + 4.0 * metrics.get("energy", 0.0)
        + 0.5 * metrics.get("heal", 0.0)
    )


def has_keyword(card: dict[str, Any], keyword: str) -> bool:
    return keyword in card.get("keywords", [])


def simulate_starter_deck(
    archetype: dict[str, Any],
    cards: dict[str, dict[str, Any]],
    samples: int,
    turns: int,
    seed: int,
) -> dict[str, float]:
    rng = random.Random(seed)
    totals: dict[str, list[float]] = defaultdict(list)
    deck_ids = [card_id for card_id in archetype.get("starting_deck", []) if card_id in cards]

    for _ in range(samples):
        draw = list(deck_ids)
        rng.shuffle(draw)
        discard: list[str] = []
        exhaust: list[str] = []
        hand: list[str] = []
        damage = block = cards_played = energy_spent = 0.0

        innate = [card_id for card_id in list(draw) if has_keyword(cards[card_id], "innate")]
        for card_id in innate:
            if len(hand) >= 10:
                break
            draw.remove(card_id)
            hand.append(card_id)
        while len(hand) < 5 and draw:
            hand.append(draw.pop())

        for turn in range(turns):
            if turn > 0:
                while len(hand) < 5:
                    if not draw:
                        draw = discard
                        discard = []
                        rng.shuffle(draw)
                    if not draw:
                        break
                    hand.append(draw.pop())

            energy = 3
            retained: list[str] = []
            while True:
                candidates: list[tuple[float, int, str]] = []
                for index, card_id in enumerate(hand):
                    card = cards[card_id]
                    cost = card.get("energy_cost", 0)
                    if not isinstance(cost, int) or isinstance(cost, bool) or cost < 0 or cost > energy:
                        continue
                    utility = card_utility(card)
                    denominator = max(1, cost)
                    candidates.append((utility / denominator, index, card_id))
                if not candidates:
                    break
                candidates.sort(reverse=True)
                _, index, card_id = candidates[0]
                card = cards[card_id]
                utility = card_utility(card)
                if utility <= 0.0:
                    break
                cost = int(card.get("energy_cost", 0))
                energy -= cost
                energy_spent += cost
                cards_played += 1
                metrics = card_metrics(card)
                damage += metrics.get("damage", 0.0)
                block += metrics.get("block", 0.0)
                hand.pop(index)
                if has_keyword(card, "exhaust"):
                    exhaust.append(card_id)
                else:
                    discard.append(card_id)

            for card_id in hand:
                card = cards[card_id]
                if has_keyword(card, "ethereal"):
                    exhaust.append(card_id)
                elif has_keyword(card, "retain"):
                    retained.append(card_id)
                else:
                    discard.append(card_id)
            hand = retained

        totals["damage_per_turn"].append(damage / turns)
        totals["block_per_turn"].append(block / turns)
        totals["cards_per_turn"].append(cards_played / turns)
        totals["energy_spent_per_turn"].append(energy_spent / turns)

    return {key: round(mean(values), 3) for key, values in totals.items()}


def enemy_phase_for_turn(enemy: dict[str, Any], turn: int, turns: int) -> dict[str, Any] | None:
    phases = enemy.get("phases")
    if not isinstance(phases, list) or not phases:
        return None
    phase_index = min(len(phases) - 1, ((turn - 1) * len(phases)) // max(1, turns))
    phase = phases[phase_index]
    return phase if isinstance(phase, dict) else None


def enemy_action_pool(enemy: dict[str, Any], turn: int, turns: int) -> list[dict[str, Any]]:
    actions = enemy.get("actions", [])
    if not isinstance(actions, list):
        return []
    phase = enemy_phase_for_turn(enemy, turn, turns)
    if phase is None:
        return [action for action in actions if isinstance(action, dict)]
    action_ids = set(phase.get("action_ids", []))
    return [action for action in actions if isinstance(action, dict) and action.get("id") in action_ids]


def action_allowed(action: dict[str, Any], player_statuses: set[str], enemy_statuses: set[str], turn: int, alive: int) -> bool:
    conditions = action.get("conditions", {})
    if not isinstance(conditions, dict):
        return True
    min_turn = conditions.get("min_turn", 1)
    max_turn = conditions.get("max_turn", 10**9)
    if isinstance(min_turn, int) and turn < min_turn:
        return False
    if isinstance(max_turn, int) and turn > max_turn:
        return False
    min_alive = conditions.get("min_alive_enemies", 1)
    max_alive = conditions.get("max_alive_enemies", 10**9)
    if isinstance(min_alive, int) and alive < min_alive:
        return False
    if isinstance(max_alive, int) and alive > max_alive:
        return False
    required_player = set(conditions.get("required_player_statuses", []))
    forbidden_player = set(conditions.get("forbidden_player_statuses", []))
    required_enemy = set(conditions.get("required_self_statuses", []))
    forbidden_enemy = set(conditions.get("forbidden_self_statuses", []))
    return (
        required_player.issubset(player_statuses)
        and not forbidden_player.intersection(player_statuses)
        and required_enemy.issubset(enemy_statuses)
        and not forbidden_enemy.intersection(enemy_statuses)
    )


def simulate_enemy(
    enemy: dict[str, Any],
    rng: random.Random,
    turns: int,
    alive: int,
) -> dict[str, float]:
    cooldowns: dict[str, int] = defaultdict(int)
    last_id = ""
    consecutive = 0
    player_statuses: set[str] = set()
    enemy_statuses: set[str] = set()
    totals: dict[str, float] = defaultdict(float)
    max_spike = 0.0

    for turn in range(1, turns + 1):
        actions = enemy_action_pool(enemy, turn, turns)
        phase = enemy_phase_for_turn(enemy, turn, turns)
        if phase is not None:
            for effect in phase.get("player_turn_effects", []):
                if isinstance(effect, dict):
                    metrics = effect_metrics(effect)
                    totals["damage"] += metrics.get("damage", 0.0)
                    totals["stress"] += metrics.get("stress", 0.0)
                    totals["energy_loss"] += metrics.get("energy_loss", 0.0)
        for action_id in list(cooldowns):
            cooldowns[action_id] = max(0, cooldowns[action_id] - 1)
        eligible: list[dict[str, Any]] = []
        for action in actions:
            action_id = str(action.get("id", ""))
            if cooldowns[action_id] > 0:
                continue
            max_consecutive = action.get("max_consecutive_uses", 10**9)
            if action_id == last_id and isinstance(max_consecutive, int) and consecutive >= max_consecutive:
                continue
            if action_allowed(action, player_statuses, enemy_statuses, turn, alive):
                eligible.append(action)
        if not eligible:
            eligible = actions
        if not eligible:
            continue
        weights = [max(1, int(action.get("weight", 1))) for action in eligible]
        action = rng.choices(eligible, weights=weights, k=1)[0]
        action_id = str(action.get("id", ""))
        consecutive = consecutive + 1 if action_id == last_id else 1
        last_id = action_id
        cooldown = action.get("cooldown", 0)
        if isinstance(cooldown, int) and cooldown > 0:
            cooldowns[action_id] = cooldown + 1

        turn_damage = 0.0
        for effect in action.get("effects", []):
            if not isinstance(effect, dict):
                continue
            metrics = effect_metrics(effect)
            turn_damage += metrics.get("damage", 0.0)
            totals["block"] += metrics.get("block", 0.0)
            totals["stress"] += metrics.get("stress", 0.0)
            totals["energy_loss"] += metrics.get("energy_loss", 0.0)
            if effect.get("type") == "apply_status":
                status = effect.get("status")
                target = effect.get("target")
                if isinstance(status, str):
                    if target in {"self", "all_enemies", "random_enemy"}:
                        enemy_statuses.add(status)
                    else:
                        player_statuses.add(status)
        totals["damage"] += turn_damage
        max_spike = max(max_spike, turn_damage)

    totals["max_spike"] = max_spike
    return dict(totals)


def load_encounters() -> list[tuple[str, str, dict[str, Any]]]:
    result: list[tuple[str, str, dict[str, Any]]] = []
    for path in sorted((DATA / "encounters").glob("*.json")):
        root = load_json(path)
        pools = root.get("pools", {}) if isinstance(root, dict) else {}
        floor_id = path.stem.replace("act", "floor").replace("_encounters", "")
        for pool_name, encounters in pools.items():
            if isinstance(encounters, list):
                for encounter in encounters:
                    if isinstance(encounter, dict):
                        result.append((floor_id, str(pool_name), encounter))
    return result


def simulate_encounter(
    encounter: dict[str, Any],
    enemies: dict[str, dict[str, Any]],
    samples: int,
    turns: int,
    seed: int,
) -> dict[str, float]:
    values: dict[str, list[float]] = defaultdict(list)
    enemy_ids = [enemy_id for enemy_id in encounter.get("enemies", []) if enemy_id in enemies]
    total_hp = sum(float(enemies[enemy_id].get("max_hp", 0)) for enemy_id in enemy_ids)
    for sample in range(samples):
        rng = random.Random(seed + sample * 7919)
        totals: dict[str, float] = defaultdict(float)
        spike = 0.0
        for enemy_id in enemy_ids:
            metrics = simulate_enemy(enemies[enemy_id], rng, turns, len(enemy_ids))
            for key, value in metrics.items():
                if key == "max_spike":
                    spike += value
                else:
                    totals[key] += value
        values["incoming_damage_per_turn"].append(totals["damage"] / turns)
        values["enemy_block_per_turn"].append(totals["block"] / turns)
        values["stress_per_turn"].append(totals["stress"] / turns)
        values["energy_loss_per_turn"].append(totals["energy_loss"] / turns)
        values["maximum_turn_spike"].append(spike)
    result = {key: round(mean(items), 3) for key, items in values.items()}
    result["total_hp"] = round(total_hp, 3)
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description="Deterministic approximate balance simulation")
    parser.add_argument("--samples", type=int, default=2000)
    parser.add_argument("--turns", type=int, default=5)
    parser.add_argument("--seed", type=int, default=1729)
    parser.add_argument("--output-dir", type=Path, default=REPORTS)
    args = parser.parse_args()
    if args.samples <= 0 or args.turns <= 0:
        raise SystemExit("--samples and --turns must be positive")

    balance_targets = load_balance_targets()
    cards = {item["id"]: item for item in load_list("cards") if isinstance(item.get("id"), str)}
    enemies = {item["id"]: item for item in load_list("enemies") if isinstance(item.get("id"), str)}
    archetypes_root = load_json(DATA / "archetypes" / "playable_archetypes.json")
    archetypes = [item for item in archetypes_root if isinstance(item, dict) and item.get("is_available", True)]

    archetype_reports: list[dict[str, Any]] = []
    for index, archetype in enumerate(archetypes):
        metrics = simulate_starter_deck(archetype, cards, args.samples, args.turns, args.seed + index * 100003)
        archetype_reports.append({"archetype_id": archetype["id"], **metrics})

    average_damage = mean(report["damage_per_turn"] for report in archetype_reports) if archetype_reports else 0.0
    average_block = mean(report["block_per_turn"] for report in archetype_reports) if archetype_reports else 0.0
    starter_target = balance_targets.get("starter_deck", {})
    target_combined_output = float(starter_target.get("target_combined_output", 22.0))
    warning_deviation = float(starter_target.get("warning_deviation_percent", 32.0))
    starter_deck_flags: list[dict[str, Any]] = []
    for report in archetype_reports:
        combined = report["damage_per_turn"] + 0.85 * report["block_per_turn"]
        deviation = 0.0 if target_combined_output <= 0 else 100.0 * (combined - target_combined_output) / target_combined_output
        report["combined_output"] = round(combined, 3)
        report["target_deviation_percent"] = round(deviation, 3)
        if abs(deviation) > warning_deviation:
            starter_deck_flags.append({
                "archetype_id": report["archetype_id"],
                "deviation_percent": round(deviation, 3),
                "direction": "high" if deviation > 0 else "low",
            })

    encounter_reports: list[dict[str, Any]] = []
    balance_flags: list[dict[str, Any]] = []
    floor_configs = balance_targets.get("floors", {})
    for index, (floor_id, pool, encounter) in enumerate(load_encounters()):
        metrics = simulate_encounter(encounter, enemies, args.samples, args.turns, args.seed + index * 31337)
        floor_config = floor_configs.get(floor_id, {}) if isinstance(floor_configs, dict) else {}
        offense_multiplier = float(floor_config.get("offense_multiplier", 1.0))
        defense_multiplier = float(floor_config.get("defense_multiplier", 1.0))
        projected_damage = average_damage * offense_multiplier
        projected_block = average_block * defense_multiplier
        turns_to_kill = metrics["total_hp"] / projected_damage if projected_damage > 0 else math.inf
        expected_hp_loss = max(0.0, metrics["incoming_damage_per_turn"] - projected_block) * turns_to_kill
        target = target_range(floor_config, pool)
        status = target_status(expected_hp_loss, target)
        report = {
            "floor_id": floor_id,
            "pool": pool,
            "encounter_id": encounter.get("id", ""),
            "enemy_count": len(encounter.get("enemies", [])),
            "weight": int(encounter.get("weight", 1)),
            "min_layer": int(encounter.get("min_layer", -1)),
            "max_layer": int(encounter.get("max_layer", -1)),
            **metrics,
            "projected_player_damage_per_turn": round(projected_damage, 3),
            "projected_player_block_per_turn": round(projected_block, 3),
            "estimated_turns_to_kill": round(turns_to_kill, 3) if math.isfinite(turns_to_kill) else None,
            "estimated_hp_loss": round(expected_hp_loss, 3) if math.isfinite(expected_hp_loss) else None,
            "hp_loss_target_min": target[0],
            "hp_loss_target_max": target[1],
            "target_status": status,
        }
        encounter_reports.append(report)
        if status == "above":
            balance_flags.append({
                "floor_id": floor_id,
                "pool": pool,
                "encounter_id": report["encounter_id"],
                "estimated_hp_loss": report["estimated_hp_loss"],
                "target_max": target[1],
                "weight": report["weight"],
                "min_layer": report["min_layer"],
            })

    floor_aggregates: list[dict[str, Any]] = []
    grouped: dict[tuple[str, str], list[dict[str, Any]]] = defaultdict(list)
    for report in encounter_reports:
        grouped[(report["floor_id"], report["pool"])].append(report)
    for (floor_id, pool), reports in sorted(grouped.items()):
        floor_aggregates.append({
            "floor_id": floor_id,
            "pool": pool,
            "encounter_count": len(reports),
            "total_weight": sum(max(1, int(r["weight"])) for r in reports),
            "average_total_hp": round(weighted_mean([(r["total_hp"], r["weight"]) for r in reports]), 3),
            "average_incoming_damage_per_turn": round(weighted_mean([(r["incoming_damage_per_turn"], r["weight"]) for r in reports]), 3),
            "average_stress_per_turn": round(weighted_mean([(r["stress_per_turn"], r["weight"]) for r in reports]), 3),
            "average_estimated_hp_loss": round(weighted_mean([(r["estimated_hp_loss"], r["weight"]) for r in reports if r["estimated_hp_loss"] is not None]), 3),
            "above_target_count": sum(1 for r in reports if r["target_status"] == "above"),
            "within_target_count": sum(1 for r in reports if r["target_status"] == "within"),
            "below_target_count": sum(1 for r in reports if r["target_status"] == "below"),
        })

    output = {
        "schema_version": 2,
        "model": "progression_adjusted_static_monte_carlo",
        "samples": args.samples,
        "turns": args.turns,
        "seed": args.seed,
        "progression_targets": balance_targets,
        "starter_decks": archetype_reports,
        "starter_deck_flags": starter_deck_flags,
        "floor_aggregates": floor_aggregates,
        "balance_flags": sorted(balance_flags, key=lambda item: (item["floor_id"], item["pool"], -float(item["estimated_hp_loss"]))),
        "encounters": encounter_reports,
    }

    args.output_dir.mkdir(parents=True, exist_ok=True)
    json_path = args.output_dir / "balance_simulation.json"
    csv_path = args.output_dir / "balance_simulation.csv"
    json_path.write_text(json.dumps(output, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    fields = [
        "floor_id", "pool", "encounter_id", "enemy_count", "total_hp",
        "incoming_damage_per_turn", "enemy_block_per_turn", "stress_per_turn",
        "energy_loss_per_turn", "maximum_turn_spike", "projected_player_damage_per_turn",
        "projected_player_block_per_turn", "estimated_turns_to_kill", "estimated_hp_loss",
        "hp_loss_target_min", "hp_loss_target_max", "target_status", "weight", "min_layer", "max_layer",
    ]
    with csv_path.open("w", encoding="utf-8", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=fields)
        writer.writeheader()
        for report in encounter_reports:
            writer.writerow({field: report.get(field) for field in fields})

    try:
        json_display = json_path.relative_to(ROOT)
        csv_display = csv_path.relative_to(ROOT)
    except ValueError:
        json_display = json_path
        csv_display = csv_path
    print(f"Wrote {json_display}")
    print(f"Wrote {csv_display}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
