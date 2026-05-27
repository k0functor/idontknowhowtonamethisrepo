#include "CombatViewModelBuilder.hpp"

#include "statuses/StatusType.hpp"
#include "drones/DroneDefinition.hpp"
#include "drones/DroneId.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>
#include <unordered_map>

namespace {
std::string localizedOrFallback(
    const LocalizationManager& localization,
    const std::string& textId,
    const std::string& fallback
) {
    const TextId id(textId);
    if (localization.hasText(id)) {
        return localization.get(id);
    }

    return fallback;
}

std::string phaseLabel(
    const LocalizationManager& localization,
    const CombatPhase phase
) {
    switch (phase) {
        case CombatPhase::NotStarted:
            return localizedOrFallback(localization, "combat.phase.not_started", "Not started");
        case CombatPhase::PlayerTurn:
            return localizedOrFallback(localization, "combat.phase.player_turn", "Player turn");
        case CombatPhase::EnemyTurn:
            return localizedOrFallback(localization, "combat.phase.enemy_turn", "Enemy turn");
        case CombatPhase::Won:
            return localizedOrFallback(localization, "combat.phase.won", "Victory");
        case CombatPhase::Lost:
            return localizedOrFallback(localization, "combat.phase.lost", "Defeat");
    }

    return localizedOrFallback(localization, "combat.phase.unknown", "Unknown");
}

std::string intentBaseLabel(
    const LocalizationManager& localization,
    const EnemyIntentType type
) {
    switch (type) {
        case EnemyIntentType::Attack:
            return localizedOrFallback(localization, "intent.attack.name", "Attack");
        case EnemyIntentType::Block:
            return localizedOrFallback(localization, "intent.block.name", "Block");
        case EnemyIntentType::Buff:
            return localizedOrFallback(localization, "intent.buff.name", "Buff");
        case EnemyIntentType::Debuff:
            return localizedOrFallback(localization, "intent.debuff.name", "Debuff");
        case EnemyIntentType::Special:
            return localizedOrFallback(localization, "intent.special.name", "Special");
        case EnemyIntentType::Unknown:
            return localizedOrFallback(localization, "intent.unknown.name", "Unknown");
    }

    return localizedOrFallback(localization, "intent.unknown.name", "Unknown");
}

std::string valueRangeText(const EnemyIntent& intent) {
    if (intent.valueMax <= 0 && intent.valueMin <= 0) {
        return {};
    }

    if (intent.valueMin == intent.valueMax) {
        return std::to_string(intent.valueMax);
    }

    return std::to_string(intent.valueMin) + "-" + std::to_string(intent.valueMax);
}

std::string intentLabel(
    const LocalizationManager& localization,
    const EnemyIntent& intent
) {
    const std::string base = intentBaseLabel(localization, intent.type);
    const std::string value = valueRangeText(intent);

    if (value.empty() || intent.type == EnemyIntentType::Buff ||
        intent.type == EnemyIntentType::Debuff || intent.type == EnemyIntentType::Special ||
        intent.type == EnemyIntentType::Unknown) {
        return base;
    }

    return base + " " + value;
}

bool startsWith(const std::string& text, const std::string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

std::string suffixAfter(const std::string& text, const std::string& prefix) {
    return text.substr(prefix.size());
}

std::string droneName(
    const LocalizationManager& localization,
    const DroneDatabase& drones,
    const std::string& droneType
) {
    const DroneId droneId(droneType);
    if (!drones.contains(droneId)) {
        return droneType;
    }

    const DroneDefinition& definition = drones.get(droneId);
    return localizedOrFallback(localization, definition.nameTextId.value, droneType);
}

std::string localizeLogEntry(
    const LocalizationManager& localization,
    const DroneDatabase& drones,
    const std::string& entry
) {
    if (entry == "Combat started") {
        return localizedOrFallback(localization, "combat.log.started", "Combat started");
    }
    if (entry == "Combat won") {
        return localizedOrFallback(localization, "combat.log.won", "Combat won");
    }
    if (entry == "Combat lost") {
        return localizedOrFallback(localization, "combat.log.lost", "Combat lost");
    }
    if (entry == "Player turn ended") {
        return localizedOrFallback(localization, "combat.log.player_turn_ended", "Player turn ended");
    }
    if (entry == "Enemy turn started") {
        return localizedOrFallback(localization, "combat.log.enemy_turn_started", "Enemy turn started");
    }
    if (entry == "Enemy turn ended") {
        return localizedOrFallback(localization, "combat.log.enemy_turn_ended", "Enemy turn ended");
    }
    if (entry == "Striker drone deals damage") {
        return localizedOrFallback(localization, "combat.log.drone_striker_damage", "Striker drone deals damage");
    }
    if (entry == "Guardian drone grants 4 block" || entry == "Guardian drone grants 4 block at end of turn") {
        return localizedOrFallback(localization, "combat.log.drone_guardian_block", "Guardian drone grants 4 block");
    }
    if (entry == "Bomber drone explodes for 8 damage to all enemies") {
        return localizedOrFallback(localization, "combat.log.drone_bomber_damage", "Bomber drone deals 8 damage to all enemies");
    }
    if (entry == "No drone to use") {
        return localizedOrFallback(localization, "combat.log.no_drone", "No drone to use");
    }

    const std::string playerTurnPrefix = "Player turn started: ";
    if (startsWith(entry, playerTurnPrefix)) {
        return localizedOrFallback(localization, "combat.log.player_turn_started", "Player turn") + ": " + suffixAfter(entry, playerTurnPrefix);
    }

    const std::string summonedDronePrefix = "Summoned drone: ";
    if (startsWith(entry, summonedDronePrefix)) {
        return localizedOrFallback(localization, "combat.log.drone_summoned", "Summoned drone") + ": " +
            droneName(localization, drones, suffixAfter(entry, summonedDronePrefix));
    }

    const std::string strikerDamagePrefix = "Striker drone deals ";
    const std::string damageSuffix = " damage";
    if (startsWith(entry, strikerDamagePrefix) && entry.size() >= strikerDamagePrefix.size() + damageSuffix.size()) {
        std::string amount = suffixAfter(entry, strikerDamagePrefix);
        if (amount.size() >= damageSuffix.size() && amount.substr(amount.size() - damageSuffix.size()) == damageSuffix) {
            amount.erase(amount.size() - damageSuffix.size());
            return localizedOrFallback(localization, "combat.log.drone_striker_damage", "Striker drone deals damage") + ": " + amount;
        }
    }

    const std::string strikerEndTurnPrefix = "Striker drone end-turn damage: ";
    if (startsWith(entry, strikerEndTurnPrefix)) {
        return localizedOrFallback(localization, "combat.log.drone_striker_damage", "Striker drone deals damage") + ": " +
            suffixAfter(entry, strikerEndTurnPrefix);
    }

    const std::string drawPrefix = "Draw cards: ";
    if (startsWith(entry, drawPrefix)) {
        return localizedOrFallback(localization, "combat.log.draw_cards", "Draw cards") + ": " + suffixAfter(entry, drawPrefix);
    }

    const std::string gainEnergyPrefix = "Gain energy: ";
    if (startsWith(entry, gainEnergyPrefix)) {
        return localizedOrFallback(localization, "combat.log.gain_energy", "Gain energy") + ": " + suffixAfter(entry, gainEnergyPrefix);
    }

    const std::string healPrefix = "Heal: ";
    if (startsWith(entry, healPrefix)) {
        return localizedOrFallback(localization, "combat.log.heal", "Heal") + ": " + suffixAfter(entry, healPrefix);
    }

    const std::string gainStressPrefix = "Gain stress: ";
    if (startsWith(entry, gainStressPrefix)) {
        return localizedOrFallback(localization, "combat.log.gain_stress", "Gain stress") + ": " + suffixAfter(entry, gainStressPrefix);
    }

    const std::string loseStressPrefix = "Lose stress: ";
    if (startsWith(entry, loseStressPrefix)) {
        return localizedOrFallback(localization, "combat.log.lose_stress", "Lose stress") + ": " + suffixAfter(entry, loseStressPrefix);
    }

    const std::string breakdownPrefix = "Stress breakdown: ";
    if (startsWith(entry, breakdownPrefix)) {
        return localizedOrFallback(localization, "combat.log.stress_breakdown", "Stress breakdown") + ": " + suffixAfter(entry, breakdownPrefix);
    }

    return entry;
}
}

CombatViewModelBuilder::CombatViewModelBuilder(
    const LocalizationManager& localization,
    const StatusDatabase& statusDatabase,
    const DroneDatabase& droneDatabase,
    const CardViewModelBuilder& cardViewModelBuilder
)
    : localization_(localization),
      statusDatabase_(statusDatabase),
      droneDatabase_(droneDatabase),
      cardViewModelBuilder_(cardViewModelBuilder) {}

CombatViewModel CombatViewModelBuilder::build(
    const CombatState& state,
    const EntityId source,
    const std::optional<EntityId> previewTarget
) const {
    return build(
        state,
        source,
        previewTarget,
        [source](const CardInstance&) {
            return source;
        }
    );
}

CombatViewModel CombatViewModelBuilder::build(
    const CombatState& state,
    const EntityId source,
    const std::optional<EntityId> previewTarget,
    const std::function<EntityId(const CardInstance&)>& sourceForCard
) const {
    (void)source;

    CombatViewModel model;
    model.phase = state.phase;
    model.turn = state.turn;
    model.energy = state.resources.energy();
    model.maxEnergy = state.resources.maxEnergy();
    model.drawPileSize = static_cast<int>(state.deck.drawPile.size());
    model.discardPileSize = static_cast<int>(state.deck.discardPile.size());
    model.exhaustPileSize = static_cast<int>(state.deck.exhaustPile.size());
    model.canEndTurn = state.phase == CombatPhase::PlayerTurn;
    model.turnLabel = localizedOrFallback(localization_, "ui.turn", "Turn");
    model.phaseText = phaseLabel(localization_, state.phase);
    model.energyLabel = localizedOrFallback(localization_, "ui.energy", "Energy");
    model.totalEnergyLabel = localizedOrFallback(localization_, "ui.total_energy", "Total energy");
    model.drawPileLabel = localizedOrFallback(localization_, "ui.draw_pile", "Draw");
    model.discardPileLabel = localizedOrFallback(localization_, "ui.discard_pile", "Discard");
    model.exhaustPileLabel = localizedOrFallback(localization_, "ui.exhaust_pile", "Exhaust");
    model.endTurnLabel = localizedOrFallback(localization_, "ui.end_turn", "End Turn");
    model.emptyLabel = localizedOrFallback(localization_, "ui.empty", "Empty");
    model.droneSlotsLabel = localizedOrFallback(localization_, "ui.drone_slots", "Drone slots");
    model.keyboardHintLabel = localizedOrFallback(
        localization_,
        "ui.combat_keyboard_hint",
        "Left/Right select card, A/D select target, Enter play"
    );

    model.players.reserve(state.players.size());
    for (const CombatEntity& player : state.players) {
        PlayerViewModel playerModel;
        playerModel.entityId = player.id;
        playerModel.name = localization_.get(player.nameTextId);
        playerModel.currentHp = player.health.current();
        playerModel.maxHp = player.health.maximum();
        playerModel.currentEnergy = state.resources.energyFor(player.id);
        playerModel.maxEnergy = state.resources.maxEnergyFor(player.id);
        playerModel.block = player.block;
        playerModel.stress = player.stress;
        playerModel.maxStress = player.maxStress;
        playerModel.stressLabel = localizedOrFallback(localization_, "ui.stress", "Stress");
        playerModel.traitIds = player.traitIds;
        playerModel.statuses = buildStatuses(player.statuses);
        playerModel.alive = player.isAlive();
        model.players.push_back(std::move(playerModel));
    }

    if (!state.players.empty()) {
        const CombatEntity& player = state.players.front();
        model.playerCurrentHp = player.health.current();
        model.playerMaxHp = player.health.maximum();
        model.playerBlock = player.block;
    }

    model.handCards.reserve(state.hand.cards().size());
    for (const CardInstance& card : state.hand.cards()) {
        model.handCards.push_back(
            cardViewModelBuilder_.build(
                state,
                card.instanceId,
                sourceForCard(card),
                previewTarget
            )
        );
    }

    std::unordered_map<std::uint64_t, EnemyIntent> intentsByEnemy;
    intentsByEnemy.reserve(state.enemyIntents.size());
    for (const EnemyIntentState& intentState : state.enemyIntents) {
        intentsByEnemy.emplace(intentState.enemyId.value, intentState.intent);
    }

    model.enemies.reserve(state.enemies.size());
    for (const CombatEntity& enemy : state.enemies) {
        EnemyViewModel enemyModel;
        enemyModel.entityId = enemy.id;
        enemyModel.name = localization_.get(enemy.nameTextId);
        enemyModel.currentHp = enemy.health.current();
        enemyModel.maxHp = enemy.health.maximum();
        enemyModel.block = enemy.block;
        enemyModel.statuses = buildStatuses(enemy.statuses);
        enemyModel.alive = enemy.isAlive();

        const auto intentIterator = intentsByEnemy.find(enemy.id.value);
        if (intentIterator != intentsByEnemy.end()) {
            enemyModel.intent = intentIterator->second;
            enemyModel.intentText = intentLabel(localization_, enemyModel.intent);
        } else {
            enemyModel.intent.type = EnemyIntentType::Unknown;
            enemyModel.intentText = enemyModel.alive ? "..." : "";
        }

        model.enemies.push_back(std::move(enemyModel));
    }

    model.recentLogEntries = recentLogEntries(state, 6);
    return model;
}

std::vector<StatusViewModel> CombatViewModelBuilder::buildStatuses(
    const StatusContainer& statuses
) const {
    std::vector<StatusViewModel> result;

    const std::vector<std::pair<std::string, int>> entries = statuses.all();
    result.reserve(entries.size());

    for (const auto& [statusId, amount] : entries) {
        StatusViewModel model;
        model.id = statusId;
        model.amount = amount;

        if (statusDatabase_.contains(StatusId(statusId))) {
            const StatusDefinition& definition = statusDatabase_.get(StatusId(statusId));
            model.name = localization_.get(definition.nameTextId);
            model.debuff = definition.type == StatusType::Debuff;
        } else {
            model.name = statusId;
            model.debuff = false;
        }

        result.push_back(std::move(model));
    }

    return result;
}

std::vector<std::string> CombatViewModelBuilder::recentLogEntries(
    const CombatState& state,
    const std::size_t maxCount
) const {
    const std::vector<std::string>& entries = state.log.entries();
    const auto begin = entries.size() <= maxCount
        ? entries.begin()
        : entries.end() - static_cast<std::ptrdiff_t>(maxCount);

    std::vector<std::string> result;
    result.reserve(static_cast<std::size_t>(std::distance(begin, entries.end())));

    for (auto iterator = begin; iterator != entries.end(); ++iterator) {
        result.push_back(localizeLogEntry(localization_, droneDatabase_, *iterator));
    }

    return result;
}
