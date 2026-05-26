#pragma once

#include "combat/CombatPhase.hpp"
#include "ui/CardViewModel.hpp"
#include "ui/ConsumableViewModel.hpp"
#include "ui/EnemyViewModel.hpp"
#include "ui/PlayerViewModel.hpp"
#include "ui/RelicViewModel.hpp"

#include <string>
#include <vector>

struct DroneSlotViewModel {
    bool filled = false;
    std::string type;
    std::string name;
    std::string description;
};

struct CombatViewModel {
    CombatPhase phase = CombatPhase::NotStarted;
    int turn = 0;

    int energy = 0;
    int maxEnergy = 0;

    int playerCurrentHp = 0;
    int playerMaxHp = 0;
    int playerBlock = 0;

    int drawPileSize = 0;
    int discardPileSize = 0;
    int exhaustPileSize = 0;

    std::string turnLabel = "Turn";
    std::string phaseText = "Not started";
    std::string energyLabel = "Energy";
    std::string totalEnergyLabel = "Total energy";
    std::string drawPileLabel = "Draw";
    std::string discardPileLabel = "Discard";
    std::string exhaustPileLabel = "Exhaust";
    std::string endTurnLabel = "End Turn";
    std::string emptyLabel = "Empty";
    std::string droneSlotsLabel = "Drone slots";

    bool canEndTurn = false;

    std::vector<PlayerViewModel> players;
    std::vector<CardViewModel> handCards;
    std::vector<EnemyViewModel> enemies;
    std::vector<std::string> recentLogEntries;
    std::vector<RelicViewModel> relics;
    std::vector<ConsumableViewModel> consumables;
    std::vector<DroneSlotViewModel> droneSlots;
};
