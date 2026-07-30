#include "telemetry/RunTelemetryWriter.hpp"

#include "data/Json.hpp"

#include <chrono>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {
std::string endReasonName(const RunEndReason reason) {
    switch (reason) {
        case RunEndReason::Victory: return "victory";
        case RunEndReason::Defeat: return "defeat";
        case RunEndReason::Abandoned: return "abandoned";
        case RunEndReason::ChallengeCompleted: return "challenge_completed";
        case RunEndReason::ChallengeFailed: return "challenge_failed";
    }
    return "unknown";
}

std::string completionTypeName(const RunCompletionType type) {
    switch (type) {
        case RunCompletionType::InProgress: return "in_progress";
        case RunCompletionType::FloorCleared: return "floor_cleared";
        case RunCompletionType::PlayableContentComplete: return "playable_content_complete";
        case RunCompletionType::Victory: return "victory";
    }
    return "unknown";
}

Json statsJson(const RunStats& stats) {
    return Json{
        {"combats_won", stats.combatsWon},
        {"combats_lost", stats.combatsLost},
        {"enemies_killed", stats.enemiesKilled},
        {"damage_taken", stats.damageTaken},
        {"damage_dealt", stats.damageDealt},
        {"damage_blocked", stats.damageBlocked},
        {"block_gained", stats.blockGained},
        {"combat_turns", stats.combatTurns},
        {"cards_played", stats.cardsPlayedInCombat},
        {"energy_spent_on_cards", stats.energySpentOnCards},
        {"maximum_single_hit", stats.maximumSingleHit},
        {"longest_combat_turns", stats.longestCombatTurns},
        {"most_cards_played_in_combat", stats.mostCardsPlayedInCombat},
        {"gold_gained", stats.goldGained},
        {"gold_spent", stats.goldSpent},
        {"cards_added", stats.cardsAdded},
        {"cards_removed", stats.cardsRemoved},
        {"cards_upgraded", stats.cardsUpgraded},
        {"relics_gained", stats.relicsGained},
        {"consumables_used", stats.consumablesUsed},
        {"active_items_used", stats.activeItemsUsed},
        {"nodes_completed", stats.nodesCompleted}
    };
}
}

void RunTelemetryWriter::append(
    const std::filesystem::path& path,
    const RunState& run,
    const RunEndReason reason
) {
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }

    std::ofstream output(path, std::ios::app);
    if (!output.is_open()) {
        throw std::runtime_error("Cannot open local run telemetry file: " + path.string());
    }

    const auto epochSeconds = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    Json record{
        {"schema_version", 1},
        {"recorded_at_unix", epochSeconds},
        {"archetype_id", run.archetypeId.value},
        {"difficulty_id", run.difficultyId.value},
        {"challenge_id", run.challengeId},
        {"seed", run.seed},
        {"end_reason", endReasonName(reason)},
        {"completion_type", completionTypeName(run.completionType)},
        {"floor_id", run.currentFloorId},
        {"floor_index", run.currentFloorIndex},
        {"deck_size", static_cast<int>(run.deckCardIds.size())},
        {"upgraded_card_count", static_cast<int>(run.upgradedDeckIndices.size())},
        {"relic_count", static_cast<int>(run.relicIds.size())},
        {"active_item_id", run.activeItem.itemId},
        {"stats", statsJson(run.stats)}
    };

    output << record.dump() << '
';
    if (!output.good()) {
        throw std::runtime_error("Cannot append local run telemetry: " + path.string());
    }
}
