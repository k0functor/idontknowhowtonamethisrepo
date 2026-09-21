#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ENEMY_DIR = ROOT / "data" / "enemies"

errors: list[str] = []
enemies: dict[str, dict] = {}
for path in sorted(ENEMY_DIR.glob("*.json")):
    for enemy in json.loads(path.read_text(encoding="utf-8")):
        enemies[enemy["id"]] = enemy

bosses = [enemy for enemy in enemies.values() if enemy.get("role") == "boss"]
phase_count = 0
summoning_phases = 0
arena_phases = 0

for boss in bosses:
    boss_id = boss["id"]
    phases = boss.get("phases", [])
    phase_count += len(phases)
    if len(phases) < 3:
        errors.append(f"{boss_id}: expected at least three boss phases")
        continue

    thresholds = [phase.get("activate_below_hp_percent") for phase in phases]
    if thresholds[0] != 100:
        errors.append(f"{boss_id}: first phase must activate at 100% HP")
    if any(not isinstance(value, int) or value < 1 or value > 100 for value in thresholds):
        errors.append(f"{boss_id}: phase thresholds must be integers in [1, 100]")
    if any(left <= right for left, right in zip(thresholds, thresholds[1:])):
        errors.append(f"{boss_id}: phase thresholds must be strictly descending")

    action_ids = {action["id"] for action in boss.get("actions", [])}
    phased_actions: set[str] = set()
    phase_pools: list[tuple[str, ...]] = []
    phase_ids: set[str] = set()
    for phase in phases:
        phase_id = phase.get("id", "")
        if not phase_id or phase_id in phase_ids:
            errors.append(f"{boss_id}: phase ids must be non-empty and unique")
        phase_ids.add(phase_id)

        pool = tuple(phase.get("action_ids", []))
        phase_pools.append(pool)
        if not pool:
            errors.append(f"{boss_id}/{phase_id}: empty phase action pool")
        unknown = set(pool) - action_ids
        if unknown:
            errors.append(f"{boss_id}/{phase_id}: unknown actions {sorted(unknown)}")
        phased_actions.update(pool)

        cap = phase.get("maximum_alive_enemies", 3)
        if not isinstance(cap, int) or cap < 1 or cap > 3:
            errors.append(f"{boss_id}/{phase_id}: maximum_alive_enemies must be in [1, 3]")

        summons = phase.get("summon_enemy_ids", [])
        if summons:
            summoning_phases += 1
        for summon_id in summons:
            if summon_id not in enemies:
                errors.append(f"{boss_id}/{phase_id}: unknown summoned enemy {summon_id}")
            elif enemies[summon_id].get("role") == "boss":
                errors.append(f"{boss_id}/{phase_id}: boss summons another boss {summon_id}")

        if phase.get("player_turn_effects"):
            arena_phases += 1

    if phased_actions != action_ids:
        errors.append(f"{boss_id}: unreachable actions {sorted(action_ids - phased_actions)}")
    if len(set(phase_pools)) < 2:
        errors.append(f"{boss_id}: every phase uses the same action pool")
    summon_phase_count = sum(bool(phase.get("summon_enemy_ids")) for phase in phases)
    arena_phase_count = sum(bool(phase.get("player_turn_effects")) for phase in phases)
    if summon_phase_count == 0 and arena_phase_count < 2:
        errors.append(f"{boss_id}: solo boss needs arena rules in at least two phases")
    if arena_phase_count == 0:
        errors.append(f"{boss_id}: no phase defines an arena rule")

for locale in ("en", "ru"):
    localization: dict[str, str] = {}
    for path in (ROOT / "data" / "localization" / locale).glob("*.json"):
        localization.update(json.loads(path.read_text(encoding="utf-8")))
    for boss in bosses:
        for phase in boss.get("phases", []):
            key = phase.get("name", "")
            if not key or key not in localization:
                errors.append(f"{locale}: missing boss phase localization {key!r}")
    for key in (
        "combat.log.boss_phase_changed",
        "combat.log.boss_arena_effect",
        "combat.log.enemy_summoned",
    ):
        if key not in localization:
            errors.append(f"{locale}: missing boss phase combat log {key}")

required_code = {
    ROOT / "src" / "combat" / "BossPhaseSystem.cpp": [
        "synchronizePhases",
        "applyPlayerTurnEffects",
        "maximumAliveEnemies",
        "enemyHpMultiplier",
        "BossPhaseChanged",
        "EnemySummoned",
    ],
    ROOT / "src" / "combat" / "EnemyMoveSelector.cpp": [
        "BossPhaseRules::actionAllowed",
    ],
    ROOT / "src" / "ui" / "EnemyView.cpp": [
        "phaseName",
        "phaseChip",
    ],
}
for path, tokens in required_code.items():
    text = path.read_text(encoding="utf-8")
    for token in tokens:
        if token not in text:
            errors.append(f"{path.relative_to(ROOT)}: missing token {token}")

if len(bosses) != 15:
    errors.append(f"expected 15 bosses, found {len(bosses)}")
if phase_count < 45:
    errors.append(f"expected at least 45 boss phases, found {phase_count}")
if summoning_phases < 13:
    errors.append(f"expected broad summon coverage, found {summoning_phases} summoning phases")
if arena_phases < 13:
    errors.append(f"expected broad arena-rule coverage, found {arena_phases} phases")

if errors:
    print("Boss phase contract failed:")
    for error in errors:
        print(f" - {error}")
    raise SystemExit(1)

print(
    f"Boss phase contract OK: {len(bosses)} bosses, {phase_count} phases, "
    f"{summoning_phases} summoning phases, {arena_phases} arena phases"
)
