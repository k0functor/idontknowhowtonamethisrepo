#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8-sig")


def main() -> int:
    state = read("src/run/RunPacingState.hpp")
    tracker = read("src/run/RunPacingTracker.cpp")
    controller = read("src/run/RunController.cpp")
    flow = read("src/flow/GameFlowController.cpp")
    serializer = read("src/save/RunStateSerializer.cpp")
    telemetry = read("src/telemetry/RunTelemetryWriter.cpp")
    report = read("tools/report_run_pacing.py")
    cmake = read("CMakeLists.txt")

    for field in (
        "activeSeconds", "combatSeconds", "rewardSeconds", "currentFloorSeconds",
        "completedRooms", "completedFloors",
    ):
        assert field in state, f"pacing state is missing {field}"

    assert "RunPacingTracker::tick" in flow
    assert "RunPacingTracker::beginRoom" in controller
    assert "RunPacingTracker::finishRoom" in controller
    assert "RunPacingTracker::finishFloor" in flow
    assert "RunPacingTracker::beginFloor" in controller
    assert "RunPhase::FloorComplete" in tracker and "RunPhase::RunComplete" in tracker

    assert '"pacing"' in serializer
    assert '"completed_rooms"' in serializer
    assert '"completed_floors"' in serializer
    assert '"phase_seconds"' in telemetry
    assert '"rooms"' in telemetry and '"floors"' in telemetry

    assert "average_active_minutes" in report
    assert "average_room_seconds" in report
    assert "room_types" in report
    assert "room_hotspots" in report
    assert "report_run_pacing" in cmake
    assert "run_pacing_contract" in cmake

    print("Run pacing contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
