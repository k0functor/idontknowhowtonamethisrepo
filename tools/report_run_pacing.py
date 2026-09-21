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
DEFAULT_OUTPUT = ROOT / "reports" / "run_pacing_summary.json"


def load_records(path: Path) -> list[dict[str, Any]]:
    if not path.exists():
        return []
    records: list[dict[str, Any]] = []
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        if not line.strip():
            continue
        try:
            value = json.loads(line)
        except json.JSONDecodeError as error:
            raise SystemExit(f"{path}:{line_number}: {error}") from error
        if isinstance(value, dict):
            records.append(value)
    return records


def rounded_mean(values: list[float]) -> float:
    return round(mean(values), 3) if values else 0.0


def main() -> int:
    parser = argparse.ArgumentParser(description="Report local run pacing from telemetry")
    parser.add_argument("--input", type=Path, default=DEFAULT_INPUT)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()

    records = load_records(args.input)
    paced = [record for record in records if isinstance(record.get("pacing"), dict)]

    floor_samples: dict[str, list[float]] = defaultdict(list)
    room_type_samples: dict[str, list[float]] = defaultdict(list)
    room_node_samples: dict[tuple[str, int, str], list[float]] = defaultdict(list)
    phase_samples: dict[str, list[float]] = defaultdict(list)
    active_samples: list[float] = []
    room_samples: list[float] = []

    for record in paced:
        pacing = record["pacing"]
        active = float(pacing.get("active_seconds", 0) or 0)
        if active > 0:
            active_samples.append(active)

        phases = pacing.get("phase_seconds", {})
        if isinstance(phases, dict):
            for phase, seconds in phases.items():
                phase_samples[str(phase)].append(float(seconds or 0))

        floors = pacing.get("floors", [])
        if isinstance(floors, list):
            for floor in floors:
                if not isinstance(floor, dict):
                    continue
                floor_id = str(floor.get("floor_id", "unknown"))
                seconds = float(floor.get("active_seconds", 0) or 0)
                if seconds >= 0:
                    floor_samples[floor_id].append(seconds)

        rooms = pacing.get("rooms", [])
        if isinstance(rooms, list):
            for room in rooms:
                if not isinstance(room, dict):
                    continue
                seconds = float(room.get("active_seconds", 0) or 0)
                room_type = str(room.get("node_type", "Unknown")).lower()
                if seconds >= 0:
                    room_samples.append(seconds)
                    room_type_samples[room_type].append(seconds)
                    room_node_samples[(str(room.get("floor_id", "unknown")), int(room.get("node_id", -1)), room_type)].append(seconds)

    floor_summary = [
        {
            "floor_id": floor_id,
            "samples": len(values),
            "average_seconds": rounded_mean(values),
            "average_minutes": round(rounded_mean(values) / 60.0, 3),
            "maximum_seconds": round(max(values), 3) if values else 0.0,
        }
        for floor_id, values in sorted(floor_samples.items())
    ]

    room_summary = [
        {
            "node_type": node_type,
            "samples": len(values),
            "average_seconds": rounded_mean(values),
            "maximum_seconds": round(max(values), 3) if values else 0.0,
        }
        for node_type, values in sorted(room_type_samples.items())
    ]

    phase_summary = {
        phase: {
            "average_seconds_per_run": rounded_mean(values),
            "average_minutes_per_run": round(rounded_mean(values) / 60.0, 3),
        }
        for phase, values in sorted(phase_samples.items())
    }

    room_hotspots = sorted(
        (
            {
                "floor_id": floor_id,
                "node_id": node_id,
                "node_type": node_type,
                "samples": len(values),
                "average_seconds": rounded_mean(values),
                "maximum_seconds": round(max(values), 3) if values else 0.0,
            }
            for (floor_id, node_id, node_type), values in room_node_samples.items()
        ),
        key=lambda item: (item["average_seconds"], item["maximum_seconds"]),
        reverse=True,
    )[:15]

    average_active_seconds = rounded_mean(active_samples)
    result = {
        "schema_version": 1,
        "record_count": len(records),
        "paced_record_count": len(paced),
        "average_active_seconds": average_active_seconds,
        "average_active_minutes": round(average_active_seconds / 60.0, 3),
        "average_room_seconds": rounded_mean(room_samples),
        "floors": floor_summary,
        "room_types": room_summary,
        "room_hotspots": room_hotspots,
        "phases": phase_summary,
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Reported pacing for {len(paced)} paced run(s) into {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
