#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAX_ENEMIES = 3


def main() -> int:
    errors: list[str] = []
    act1_has_three = False

    for path in sorted((ROOT / "data" / "encounters").glob("*.json")):
        root = json.loads(path.read_text(encoding="utf-8"))
        pools = root.get("pools", {})
        file_has_group = False

        for pool_name, encounters in pools.items():
            for index, encounter in enumerate(encounters):
                owner = f"{path.relative_to(ROOT)}:{pool_name}[{index}]"
                enemies = encounter.get("enemies")
                if not isinstance(enemies, list) or not enemies:
                    errors.append(f"{owner}: enemies must be a non-empty array")
                    continue
                if len(enemies) > MAX_ENEMIES:
                    errors.append(f"{owner}: contains {len(enemies)} enemies; maximum is {MAX_ENEMIES}")
                file_has_group = file_has_group or len(enemies) > 1
                if path.name == "act1_encounters.json" and len(enemies) == 3:
                    act1_has_three = True

        if not file_has_group:
            errors.append(f"{path.relative_to(ROOT)}: no multi-enemy encounter is defined")

    if not act1_has_three:
        errors.append("data/encounters/act1_encounters.json: no three-enemy encounter exercises the compact layout")

    if errors:
        print("Encounter contract test failed:")
        for error in errors:
            print(f" - {error}")
        return 1

    print("Encounter contract test passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
