#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "simulate_balance.py"


def run(output: Path) -> dict:
    subprocess.run(
        [sys.executable, str(TOOL), "--samples", "24", "--turns", "3", "--seed", "12345", "--output-dir", str(output)],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    return json.loads((output / "balance_simulation.json").read_text(encoding="utf-8"))


def main() -> int:
    with tempfile.TemporaryDirectory() as first_raw, tempfile.TemporaryDirectory() as second_raw:
        first = run(Path(first_raw))
        second = run(Path(second_raw))
        assert first == second, "balance simulation must be deterministic for the same seed"
        assert first["schema_version"] == 2
        assert len(first["starter_decks"]) == 7
        assert len(first["encounters"]) >= 200
        assert first["floor_aggregates"]
        assert first["progression_targets"]["schema_version"] == 1
        assert isinstance(first["balance_flags"], list)
        for report in first["starter_decks"]:
            assert report["damage_per_turn"] >= 0
            assert report["block_per_turn"] >= 0
            assert report["cards_per_turn"] > 0
            assert "combined_output" in report
            assert "target_deviation_percent" in report
        for report in first["encounters"]:
            assert report["enemy_count"] in {1, 2, 3}
            assert report["total_hp"] > 0
            assert report["incoming_damage_per_turn"] >= 0
            assert report["target_status"] in {"below", "within", "above"}
            assert report["projected_player_damage_per_turn"] > 0
            assert report["projected_player_block_per_turn"] >= 0
    print("Balance simulation contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
