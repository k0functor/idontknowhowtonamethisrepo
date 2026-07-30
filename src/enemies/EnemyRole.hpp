#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

enum class EnemyRole {
    Striker,
    Defender,
    Support,
    Controller,
    Bruiser,
    Minion,
    Boss
};

inline std::string toString(const EnemyRole role) {
    switch (role) {
        case EnemyRole::Striker: return "striker";
        case EnemyRole::Defender: return "defender";
        case EnemyRole::Support: return "support";
        case EnemyRole::Controller: return "controller";
        case EnemyRole::Bruiser: return "bruiser";
        case EnemyRole::Minion: return "minion";
        case EnemyRole::Boss: return "boss";
    }

    throw std::runtime_error("Unknown EnemyRole");
}

inline EnemyRole enemyRoleFromString(const std::string_view value) {
    if (value == "striker") return EnemyRole::Striker;
    if (value == "defender") return EnemyRole::Defender;
    if (value == "support") return EnemyRole::Support;
    if (value == "controller") return EnemyRole::Controller;
    if (value == "bruiser") return EnemyRole::Bruiser;
    if (value == "minion") return EnemyRole::Minion;
    if (value == "boss") return EnemyRole::Boss;

    throw std::runtime_error("Unknown enemy role: " + std::string(value));
}
