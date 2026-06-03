#pragma once

#include "entities/EntityId.hpp"
#include "ui/RelicViewModel.hpp"
#include "ui/StatusViewModel.hpp"

#include <raylib.h>

#include <string>
#include <vector>

struct PlayerViewModel {
    EntityId entityId;

    std::string name;
    std::string definitionId;

    int currentHp = 1;
    int maxHp = 1;
    int currentEnergy = 0;
    int maxEnergy = 0;
    int block = 0;
    int stress = 0;
    int maxStress = 200;

    std::string blockLabel = {};
    std::string stressLabel = {};
    std::string activeTurnLabel = {};
    std::string activeStanceLabel = {};
    std::string activeStanceName = {};
    std::string activeStanceDescription = {};
    std::string stanceShiftBonusLabel = {};
    std::string stressPowerLabel = {};
    std::string stressPowerDescription = {};
    std::string relicsLabel = {};
    int stressPowerDamageBonus = 0;
    int stressPowerNextThreshold = 0;

    std::vector<StatusViewModel> statuses;
    std::vector<RelicViewModel> relics;
    std::vector<std::string> traitIds;

    Vector2 renderOffset{0.f, 0.f};

    bool alive = true;
    bool activeTurn = false;
    bool targetable = false;
    bool previewTarget = false;
};
