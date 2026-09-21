#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "simulate_full_runs.py"


def main() -> int:
    assert TOOL.exists(), "full-run simulator tool is missing"
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    assert "simulate_full_runs.py" in cmake, "full-run simulator is not wired into CMake"

    shared_cards = json.loads((ROOT / "data" / "cards" / "sadist_masochist_cards.json").read_text(encoding="utf-8"))
    assert shared_cards, "sadist/masochist card pool is empty"
    assert all(card.get("card_pool") == "sadist_masochist" for card in shared_cards), (
        "shared sadist/masochist cards must explicitly belong to the archetype reward pool"
    )
    assert sum(card.get("rarity") not in {"starter", "special"} for card in shared_cards) >= 20, (
        "sadist/masochist need a real generated reward pool"
    )

    with tempfile.TemporaryDirectory() as temp_dir:
        output = Path(temp_dir) / "summary.json"
        csv_output = Path(temp_dir) / "runs.csv"
        subprocess.run(
            [
                sys.executable,
                str(TOOL),
                "--runs", "2",
                "--encounter-samples", "1",
                "--turns", "3",
                "--seed", "17",
                "--attrition-scale", "0.42",
                "--output", str(output),
                "--csv", str(csv_output),
            ],
            cwd=ROOT,
            check=True,
            stdout=subprocess.DEVNULL,
        )
        report = json.loads(output.read_text(encoding="utf-8"))
        assert report["schema_version"] == 1
        assert report["model"] == "heuristic_full_run_monte_carlo"
        assert report["runs_per_archetype"] == 2
        assert report["total_runs"] == 14
        assert len(report["archetypes"]) == 7
        assert len(report["floor_survival"]) == 5
        assert report["limitations"], "approximate simulator must state its limitations"
        assert report["attrition_scale"] == 0.42
        assert csv_output.exists() and csv_output.stat().st_size > 0

    print("Full-run simulation contract: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
