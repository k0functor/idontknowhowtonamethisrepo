#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from collections import defaultdict
from pathlib import Path
from statistics import mean
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_INPUT = ROOT / "saves" / "telemetry" / "run_history.jsonl"
DEFAULT_OUTPUT = ROOT / "reports" / "run_telemetry_summary.json"


def main() -> int:
    parser = argparse.ArgumentParser(description="Summarize local run telemetry")
    parser.add_argument("--input", type=Path, default=DEFAULT_INPUT)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()

    if not args.input.exists():
        print(f"No local run telemetry found at {args.input}")
        return 0

    records: list[dict[str, Any]] = []
    for line_number, line in enumerate(args.input.read_text(encoding="utf-8").splitlines(), start=1):
        if not line.strip():
            continue
        try:
            record = json.loads(line)
        except json.JSONDecodeError as error:
            raise SystemExit(f"{args.input}:{line_number}: {error}") from error
        if isinstance(record, dict):
            records.append(record)

    grouped: dict[tuple[str, str], list[dict[str, Any]]] = defaultdict(list)
    for record in records:
        grouped[(str(record.get("archetype_id", "unknown")), str(record.get("difficulty_id", "unknown")))].append(record)

    groups: list[dict[str, Any]] = []
    for (archetype_id, difficulty_id), items in sorted(grouped.items()):
        victories = sum(record.get("end_reason") in {"victory", "challenge_completed"} for record in items)
        stats = [record.get("stats", {}) for record in items if isinstance(record.get("stats"), dict)]
        groups.append({
            "archetype_id": archetype_id,
            "difficulty_id": difficulty_id,
            "runs": len(items),
            "win_rate": round(victories / len(items), 4) if items else 0.0,
            "average_damage_taken": round(mean(float(item.get("damage_taken", 0)) for item in stats), 3) if stats else 0.0,
            "average_damage_dealt": round(mean(float(item.get("damage_dealt", 0)) for item in stats), 3) if stats else 0.0,
            "average_combat_turns": round(mean(float(item.get("combat_turns", 0)) for item in stats), 3) if stats else 0.0,
            "average_cards_played": round(mean(float(item.get("cards_played", 0)) for item in stats), 3) if stats else 0.0,
            "average_nodes_completed": round(mean(float(item.get("nodes_completed", 0)) for item in stats), 3) if stats else 0.0,
        })

    result = {"schema_version": 1, "record_count": len(records), "groups": groups}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Summarized {len(records)} run(s) into {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
