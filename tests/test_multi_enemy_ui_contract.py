#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def require(text: str, needle: str, owner: str, errors: list[str]) -> None:
    if needle not in text:
        errors.append(f"{owner}: missing {needle!r}")


def main() -> int:
    errors: list[str] = []

    enemy_view = (ROOT / "src/ui/EnemyView.cpp").read_text(encoding="utf-8")
    combat_view = (ROOT / "src/ui/CombatView.cpp").read_text(encoding="utf-8")
    combat_scene = (ROOT / "src/scenes/CombatScene.cpp").read_text(encoding="utf-8")
    scene_header = (ROOT / "src/scenes/CombatScene.hpp").read_text(encoding="utf-8")

    require(enemy_view, "intentBounds()", "EnemyView.cpp", errors)
    require(enemy_view, "drawCornerReticle", "EnemyView.cpp", errors)
    require(enemy_view, "model_.formationLabel", "EnemyView.cpp", errors)
    require(enemy_view, "visibleStatusCapacity", "EnemyView.cpp", errors)
    require(enemy_view, "position_.x + model_.renderOffset.x", "EnemyView.cpp", errors)

    require(combat_view, "enemyCount == 3", "CombatView.cpp", errors)
    require(combat_view, "verticalOffset", "CombatView.cpp", errors)
    require(combat_view, "minimumBodyY", "CombatView.cpp", errors)

    require(scene_header, "GroupImpactAnimation", "CombatScene.hpp", errors)
    require(combat_scene, "enqueueGroupImpactAnimation", "CombatScene.cpp", errors)
    require(combat_scene, "renderGroupImpactAnimations", "CombatScene.cpp", errors)
    require(combat_scene, "sanitizeTargetSelection", "CombatScene.cpp", errors)
    require(combat_scene, "cardAffectsAllEnemies", "CombatScene.cpp", errors)

    required_localization = {
        "ui.intent_scope.party",
        "ui.intent_scope.enemy_team",
        "ui.intent_scope.party_and_team",
        "ui.enemy.defeated",
    }
    for locale in ("en", "ru"):
        data = json.loads((ROOT / f"data/localization/{locale}/core.json").read_text(encoding="utf-8"))
        missing = sorted(required_localization - data.keys())
        if missing:
            errors.append(f"{locale}/core.json: missing keys {missing}")

    if errors:
        print("Multi-enemy UI contract failed:")
        for error in errors:
            print(f" - {error}")
        return 1

    print("Multi-enemy UI contract passed: intent panels, formation layout, group impacts and target cleanup are present")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
