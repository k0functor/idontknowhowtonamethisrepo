#pragma once

#include "entities/EntityId.hpp"
#include "enemies/EnemyIntent.hpp"
#include "ui/StatusViewModel.hpp"

#include <raylib.h>

#include <string>
#include <vector>

struct EnemyViewModel {
    EntityId entityId;

    std::string name;
    std::string phaseName;

    int currentHp = 1;
    int maxHp = 1;
    int block = 0;

    std::string blockLabel = {};

    EnemyIntent intent;
    std::string intentText;
    std::string intentDetailText;
    std::string intentScopeLabel;
    std::string formationLabel;
    std::string defeatedLabel;

    std::vector<StatusViewModel> statuses;

    bool alive = true;
    float opacity = 1.f;
    Vector2 renderOffset{0.f, 0.f};
    bool targetable = false;
    bool previewTarget = false;
    bool intentAffectsMultipleTargets = false;
    int intentDangerLevel = 0;
};
