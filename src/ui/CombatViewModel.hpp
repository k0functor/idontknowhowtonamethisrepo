#pragma once

#include "combat/CombatPhase.hpp"
#include "ui/CardViewModel.hpp"
#include "ui/ConsumableViewModel.hpp"
#include "ui/EnemyViewModel.hpp"
#include "ui/PlayerViewModel.hpp"
#include "ui/RelicViewModel.hpp"

#include <cstdint>
#include <string>
#include <vector>


enum class CombatJournalTone {
    Neutral,
    Damage,
    Defense,
    Status,
    Stress,
    Resource,
    Important
};

struct CombatJournalEntryViewModel {
    std::uint64_t sequence = 0;
    CombatJournalTone tone = CombatJournalTone::Neutral;
    std::string text;
    std::string detail;
};

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

    int incomingDamageMin = 0;
    int incomingDamageMax = 0;
    int attackingEnemyCount = 0;
    int partyWideThreatCount = 0;

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
    std::string targetHintLabel;
    std::string turnOrderLabel;
    std::string activeActorLabel;
    std::string incomingDamageLabel;
    std::string partyWideThreatLabel;

    bool canEndTurn = false;
    bool showTopRelics = true;

    std::vector<PlayerViewModel> players;
    std::vector<CardViewModel> handCards;
    std::vector<EnemyViewModel> enemies;
    std::vector<CombatJournalEntryViewModel> recentJournalEntries;
    std::vector<RelicViewModel> relics;
    std::vector<ConsumableViewModel> consumables;
    std::vector<DroneSlotViewModel> droneSlots;
};
