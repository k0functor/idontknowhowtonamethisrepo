#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

enum class CombatLogEntryType {
    Text,
    CombatStarted,
    CombatWon,
    CombatLost,
    PlayerTurnStarted,
    PlayerTurnEnded,
    EnemyTurnStarted,
    EnemyTurnEnded,
    ActivePlayerActor,
    CardPlayed,
    CannotPlayCard,
    EnemyAction,
    BossPhaseChanged,
    BossArenaEffect,
    EnemySummoned,
    DamageDealt,
    BlockGained,
    Heal,
    DrawCards,
    DiscardCards,
    GainEnergy,
    LoseEnergy,
    LoseHp,
    StatusApplied,
    PoisonDamage,
    BurnDamage,
    GainStress,
    LoseStress,
    StressResolve,
    StressBreakdown,
    StressBreakdownPrevented,
    StressCollapse,
    StressBreakdownDiscard,
    StressBreakdownEnergy,
    StressBreakdownStatusCards,
    StressBreakdownCost,
    StressBreakdownForcedCard,
    MonkStanceShiftReward,
    DroneSummoned,
    NoDrone,
    DroneNoActiveAction,
    DroneNoReadyAction,
    DroneAction,
    DroneConsumed,
    UsedConsumable,
    RelicTriggered,
    SadistHurtsMasochist,
    MasochistPainBonus,
    EffectNotImplemented
};

struct CombatLogEntry {
    using Variables = std::unordered_map<std::string, std::string>;

    CombatLogEntryType type = CombatLogEntryType::Text;
    Variables variables;
    std::string text;
    std::uint64_t sequence = 0;

    static CombatLogEntry make(CombatLogEntryType type, Variables variables = {}) {
        CombatLogEntry entry;
        entry.type = type;
        entry.variables = std::move(variables);
        return entry;
    }

    static CombatLogEntry makeText(std::string text) {
        CombatLogEntry entry;
        entry.type = CombatLogEntryType::Text;
        entry.text = std::move(text);
        return entry;
    }
};

class CombatLog {
public:
    void add(CombatLogEntry entry);
    void add(CombatLogEntryType type, CombatLogEntry::Variables variables = {});
    void addText(std::string text);
    void clear();

    const std::vector<CombatLogEntry>& entries() const;

private:
    static constexpr std::size_t maxEntries_ = 256u;

    std::vector<CombatLogEntry> entries_;
    std::uint64_t nextSequence_ = 1u;
};
