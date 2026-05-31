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
    bool cardActivationAvailable = false;
    std::string type;
    std::string name;
    std::string description;
    std::string cardActivationLabel;
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

    std::string turnLabel = {};
    std::string phaseText = {};
    std::string energyLabel = {};
    std::string totalEnergyLabel = {};
    std::string drawPileLabel = {};
    std::string discardPileLabel = {};
    std::string exhaustPileLabel = {};
    std::string endTurnLabel = {};
    std::string emptyLabel = {};
    std::string droneSlotsLabel = {};
    std::string keyboardHintLabel;
    std::string turnOrderLabel;
    std::string activeActorLabel;

    bool canEndTurn = false;

    std::vector<PlayerViewModel> players;
    std::vector<CardViewModel> handCards;
    std::vector<EnemyViewModel> enemies;
    std::vector<std::string> recentLogEntries;
    std::vector<RelicViewModel> relics;
    std::vector<ConsumableViewModel> consumables;
    std::vector<DroneSlotViewModel> droneSlots;
};
