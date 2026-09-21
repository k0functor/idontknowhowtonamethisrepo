#include "run/RunPacingTracker.hpp"

#include "run/RunPhase.hpp"
#include "run/RunState.hpp"

#include <algorithm>

namespace {
float safeDelta(const float deltaSeconds) {
    return std::clamp(deltaSeconds, 0.f, 0.25f);
}

void addPhaseSeconds(RunPacingState& pacing, const RunPhase phase, const float seconds) {
    switch (phase) {
        case RunPhase::Map: pacing.mapSeconds += seconds; break;
        case RunPhase::Combat: pacing.combatSeconds += seconds; break;
        case RunPhase::Reward: pacing.rewardSeconds += seconds; break;
        case RunPhase::Event: pacing.eventSeconds += seconds; break;
        case RunPhase::Shop: pacing.shopSeconds += seconds; break;
        case RunPhase::Rest: pacing.restSeconds += seconds; break;
        case RunPhase::Chest: pacing.chestSeconds += seconds; break;
        case RunPhase::FloorComplete:
        case RunPhase::RunComplete:
            break;
    }
}
}

void RunPacingTracker::tick(RunState& run, const float deltaSeconds) {
    if (run.phase == RunPhase::FloorComplete || run.phase == RunPhase::RunComplete) {
        return;
    }

    const float seconds = safeDelta(deltaSeconds);
    if (seconds <= 0.f) {
        return;
    }

    RunPacingState& pacing = run.pacing;
    pacing.activeSeconds += seconds;
    addPhaseSeconds(pacing, run.phase, seconds);

    if (pacing.floorActive) {
        pacing.currentFloorSeconds += seconds;
    }
    if (pacing.roomActive) {
        pacing.currentRoomSeconds += seconds;
    }
}

void RunPacingTracker::beginRoom(RunState& run, const int nodeId, const RunMapNodeType nodeType) {
    RunPacingState& pacing = run.pacing;
    if (pacing.roomActive && pacing.currentRoomNodeId == nodeId) {
        return;
    }

    pacing.roomActive = true;
    pacing.currentRoomNodeId = nodeId;
    pacing.currentRoomType = nodeType;
    pacing.currentRoomSeconds = 0.f;
}

void RunPacingTracker::finishRoom(RunState& run, const int nodeId) {
    RunPacingState& pacing = run.pacing;
    if (!pacing.roomActive || pacing.currentRoomNodeId != nodeId) {
        return;
    }

    pacing.completedRooms.push_back(RunRoomTiming{
        run.currentFloorId,
        run.currentFloorIndex,
        nodeId,
        pacing.currentRoomType,
        pacing.currentRoomSeconds
    });
    pacing.roomActive = false;
    pacing.currentRoomNodeId = -1;
    pacing.currentRoomSeconds = 0.f;
}

void RunPacingTracker::finishFloor(RunState& run) {
    RunPacingState& pacing = run.pacing;
    if (!pacing.floorActive) {
        return;
    }

    pacing.completedFloors.push_back(RunFloorTiming{
        run.currentFloorId,
        run.currentFloorIndex,
        pacing.currentFloorSeconds,
        std::max(0, run.stats.nodesCompleted - pacing.floorStartNodesCompleted)
    });
    pacing.currentFloorSeconds = 0.f;
    pacing.floorActive = false;
}

void RunPacingTracker::beginFloor(RunState& run) {
    RunPacingState& pacing = run.pacing;
    pacing.currentFloorSeconds = 0.f;
    pacing.floorStartNodesCompleted = run.stats.nodesCompleted;
    pacing.floorActive = true;
    pacing.roomActive = false;
    pacing.currentRoomNodeId = -1;
    pacing.currentRoomSeconds = 0.f;
}
