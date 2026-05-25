#include "CombatState.hpp"

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
