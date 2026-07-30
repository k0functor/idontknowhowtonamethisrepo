#include "CombatState.hpp"

#include <algorithm>
#include <stdexcept>

CombatEntity& CombatState::entity(const EntityId id) {
    for (CombatEntity& current : players) {
        if (current.id == id) {
            return current;
        }
    }

    for (CombatEntity& current : enemies) {
        if (current.id == id) {
            return current;
        }
    }

    throw std::runtime_error("Unknown combat entity");
}

const CombatEntity& CombatState::entity(const EntityId id) const {
    for (const CombatEntity& current : players) {
        if (current.id == id) {
            return current;
        }
    }

    for (const CombatEntity& current : enemies) {
        if (current.id == id) {
            return current;
        }
    }

    throw std::runtime_error("Unknown combat entity");
}

bool CombatState::hasEntity(const EntityId id) const {
    for (const CombatEntity& current : players) {
        if (current.id == id) {
            return true;
        }
    }

    for (const CombatEntity& current : enemies) {
        if (current.id == id) {
            return true;
        }
    }

    return false;
}

bool CombatState::isEnemy(const EntityId id) const {
    for (const CombatEntity& current : enemies) {
        if (current.id == id) {
            return true;
        }
    }

    return false;
}

bool CombatState::isPlayer(const EntityId id) const {
    for (const CombatEntity& current : players) {
        if (current.id == id) {
            return true;
        }
    }

    return false;
}

std::vector<EntityId> CombatState::aliveEnemyIds() const {
    std::vector<EntityId> result;

    for (const CombatEntity& enemy : enemies) {
        if (enemy.isAlive()) {
            result.push_back(enemy.id);
        }
    }

    return result;
}

std::vector<EntityId> CombatState::alivePlayerIds() const {
    std::vector<EntityId> result;

    for (const CombatEntity& player : players) {
        if (player.isAlive()) {
            result.push_back(player.id);
        }
    }

    return result;
}

std::size_t CombatState::aliveEnemyCount() const {
    return static_cast<std::size_t>(std::count_if(enemies.begin(), enemies.end(), [](const CombatEntity& enemy) {
        return enemy.isAlive();
    }));
}

std::size_t CombatState::alivePlayerCount() const {
    return static_cast<std::size_t>(std::count_if(players.begin(), players.end(), [](const CombatEntity& player) {
        return player.isAlive();
    }));
}

void CombatState::pruneEnemyIntents() {
    enemyIntents.erase(
        std::remove_if(
            enemyIntents.begin(),
            enemyIntents.end(),
            [this](const EnemyIntentState& intentState) {
                return !hasEntity(intentState.enemyId) ||
                       !isEnemy(intentState.enemyId) ||
                       !entity(intentState.enemyId).isAlive();
            }
        ),
        enemyIntents.end()
    );
}

std::optional<EntityId> CombatState::activePlayerId() const {
    const CombatEntity* current = activePlayer();
    if (current == nullptr) {
        return std::nullopt;
    }

    return current->id;
}

CombatEntity* CombatState::activePlayer() {
    if (players.empty() || activePlayerIndex >= players.size()) {
        return nullptr;
    }

    if (!players[activePlayerIndex].isAlive()) {
        return nullptr;
    }

    return &players[activePlayerIndex];
}

const CombatEntity* CombatState::activePlayer() const {
    if (players.empty() || activePlayerIndex >= players.size()) {
        return nullptr;
    }

    if (!players[activePlayerIndex].isAlive()) {
        return nullptr;
    }

    return &players[activePlayerIndex];
}


EntityId CombatState::createDynamicEntityId() const {
    std::uint64_t maximum = 0;
    for (const CombatEntity& player : players) {
        maximum = std::max(maximum, player.id.value);
    }
    for (const CombatEntity& enemy : enemies) {
        maximum = std::max(maximum, enemy.id.value);
    }
    return EntityId{maximum + 1};
}

void CombatState::rememberStatusSeen(const std::string& statusId) {
    if (statusId.empty()) {
        return;
    }

    if (std::find(seenStatusIds.begin(), seenStatusIds.end(), statusId) == seenStatusIds.end()) {
        seenStatusIds.push_back(statusId);
    }
}

int CombatState::cardCostModifier(const EntityId owner) const {
    const auto iterator = turnCardCostModifiers_.find(owner.value);
    return iterator == turnCardCostModifiers_.end() ? 0 : iterator->second;
}

void CombatState::setCardCostModifier(const EntityId owner, const int amount) {
    if (amount == 0) {
        turnCardCostModifiers_.erase(owner.value);
        return;
    }

    turnCardCostModifiers_[owner.value] = amount;
}

void CombatState::clearTurnCardCostModifiers() {
    turnCardCostModifiers_.clear();
}


void CombatState::primeStressBreakdown(const EntityId target, std::string breakdownType) {
    if (breakdownType.empty()) {
        primedStressBreakdowns_.erase(target.value);
        return;
    }
    primedStressBreakdowns_[target.value] = std::move(breakdownType);
}

std::optional<std::string> CombatState::consumePrimedStressBreakdown(const EntityId target) {
    const auto iterator = primedStressBreakdowns_.find(target.value);
    if (iterator == primedStressBreakdowns_.end()) {
        return std::nullopt;
    }
    std::string result = std::move(iterator->second);
    primedStressBreakdowns_.erase(iterator);
    return result;
}

std::optional<std::string> CombatState::primedStressBreakdown(const EntityId target) const {
    const auto iterator = primedStressBreakdowns_.find(target.value);
    return iterator == primedStressBreakdowns_.end() ? std::nullopt : std::optional<std::string>(iterator->second);
}

void CombatState::armStressBreakdownGuard(const EntityId target) {
    stressBreakdownGuards_[target.value] = true;
}

bool CombatState::hasStressBreakdownGuard(const EntityId target) const {
    const auto iterator = stressBreakdownGuards_.find(target.value);
    return iterator != stressBreakdownGuards_.end() && iterator->second;
}

bool CombatState::consumeStressBreakdownGuard(const EntityId target) {
    const auto iterator = stressBreakdownGuards_.find(target.value);
    if (iterator == stressBreakdownGuards_.end() || !iterator->second) {
        return false;
    }
    stressBreakdownGuards_.erase(iterator);
    return true;
}
