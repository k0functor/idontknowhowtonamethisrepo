#pragma once

#include "run/RunMapNodeType.hpp"

#include <string>
#include <vector>

struct RunRoomTiming {
    std::string floorId;
    int floorIndex = 0;
    int nodeId = -1;
    RunMapNodeType nodeType = RunMapNodeType::Combat;
    float activeSeconds = 0.f;
};

struct RunFloorTiming {
    std::string floorId;
    int floorIndex = 0;
    float activeSeconds = 0.f;
    int roomsCompleted = 0;
};

struct RunPacingState {
    float activeSeconds = 0.f;
    float mapSeconds = 0.f;
    float combatSeconds = 0.f;
    float rewardSeconds = 0.f;
    float eventSeconds = 0.f;
    float shopSeconds = 0.f;
    float restSeconds = 0.f;
    float chestSeconds = 0.f;

    float currentFloorSeconds = 0.f;
    int floorStartNodesCompleted = 0;
    bool floorActive = true;

    bool roomActive = false;
    int currentRoomNodeId = -1;
    RunMapNodeType currentRoomType = RunMapNodeType::Combat;
    float currentRoomSeconds = 0.f;

    std::vector<RunRoomTiming> completedRooms;
    std::vector<RunFloorTiming> completedFloors;
};
