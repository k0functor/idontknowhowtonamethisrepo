#pragma once

#include "ui/CardViewModel.hpp"
#include "ui/EnemyViewModel.hpp"

#include <string>
#include <vector>

struct CombatViewModel {
    int energy = 0;
    int maxEnergy = 0;

    int drawPileSize = 0;
    int discardPileSize = 0;
    int exhaustPileSize = 0;

    std::vector<CardViewModel> handCards;
    std::vector<EnemyViewModel> enemies;
    std::vector<std::string> recentLogEntries;
};
