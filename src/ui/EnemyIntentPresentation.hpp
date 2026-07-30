#pragma once

#include "enemies/EnemyIntent.hpp"

struct EnemyIntentPresentation {
    bool affectsAllPlayers = false;
    bool affectsEnemyTeam = false;
    bool affectsMultipleTargets = false;
};

EnemyIntentPresentation summarizeEnemyIntent(const EnemyIntent& intent);
