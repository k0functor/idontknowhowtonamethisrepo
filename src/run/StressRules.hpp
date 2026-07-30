#pragma once

#include "core/Random.hpp"

#include <algorithm>
#include <string>

namespace StressRules {
inline constexpr int CalmThreshold = 0;
inline constexpr int TenseThreshold = 40;
inline constexpr int PressuredThreshold = 80;
inline constexpr int PanickedThreshold = 120;
inline constexpr int BreakingThreshold = 160;
inline constexpr int ResolveThreshold = 100;
inline constexpr int MaximumStress = 200;
inline constexpr double PositiveResolveChance = 0.30;

inline constexpr const char* BreakdownTraitId = "stress_breakdown";
inline constexpr const char* ResolveTraitId = "stress_resolve";

enum class StressBand {
    Calm,
    Tense,
    Pressured,
    Panicked,
    Breaking,
    Collapsed
};

inline StressBand bandForStress(const int stress) {
    if (stress >= MaximumStress) {
        return StressBand::Collapsed;
    }
    if (stress >= BreakingThreshold) {
        return StressBand::Breaking;
    }
    if (stress >= PanickedThreshold) {
        return StressBand::Panicked;
    }
    if (stress >= PressuredThreshold) {
        return StressBand::Pressured;
    }
    if (stress >= TenseThreshold) {
        return StressBand::Tense;
    }
    return StressBand::Calm;
}

inline int minimumForBand(const StressBand band) {
    switch (band) {
        case StressBand::Calm: return CalmThreshold;
        case StressBand::Tense: return TenseThreshold;
        case StressBand::Pressured: return PressuredThreshold;
        case StressBand::Panicked: return PanickedThreshold;
        case StressBand::Breaking: return BreakingThreshold;
        case StressBand::Collapsed: return MaximumStress;
    }
    return CalmThreshold;
}

inline int nextBandThreshold(const int stress) {
    switch (bandForStress(stress)) {
        case StressBand::Calm: return TenseThreshold;
        case StressBand::Tense: return PressuredThreshold;
        case StressBand::Pressured: return PanickedThreshold;
        case StressBand::Panicked: return BreakingThreshold;
        case StressBand::Breaking: return MaximumStress;
        case StressBand::Collapsed: return 0;
    }
    return 0;
}

inline const char* bandLocalizationSuffix(const StressBand band) {
    switch (band) {
        case StressBand::Calm: return "calm";
        case StressBand::Tense: return "tense";
        case StressBand::Pressured: return "pressured";
        case StressBand::Panicked: return "panicked";
        case StressBand::Breaking: return "breaking";
        case StressBand::Collapsed: return "collapsed";
    }
    return "calm";
}

enum class ResolveOutcome {
    None,
    Breakdown,
    Resolve
};

struct StressAdjustmentResult {
    int before = 0;
    int after = 0;
    int applied = 0;
    StressBand beforeBand = StressBand::Calm;
    StressBand afterBand = StressBand::Calm;
    bool bandChanged = false;
    bool resolveCheckTriggered = false;
    ResolveOutcome resolveOutcome = ResolveOutcome::None;
    bool collapsed = false;
};

template <typename ActorState>
bool hasTrait(const ActorState& actor, const std::string& traitId) {
    return std::find(actor.traitIds.begin(), actor.traitIds.end(), traitId) != actor.traitIds.end();
}

template <typename ActorState>
void addTraitIfMissing(ActorState& actor, const std::string& traitId) {
    if (!hasTrait(actor, traitId)) {
        actor.traitIds.push_back(traitId);
    }
}

template <typename ActorState>
void removeTrait(ActorState& actor, const std::string& traitId) {
    actor.traitIds.erase(
        std::remove(actor.traitIds.begin(), actor.traitIds.end(), traitId),
        actor.traitIds.end()
    );
}

template <typename ActorState>
void normalize(ActorState& actor) {
    actor.maxStress = std::max(MaximumStress, actor.maxStress);
    actor.stress = std::clamp(actor.stress, 0, actor.maxStress);

    if (actor.stress < ResolveThreshold) {
        actor.resolveCheckTriggered = false;
    }
}

template <typename ActorState>
StressAdjustmentResult applyDelta(ActorState& actor, const int delta, Random* random) {
    normalize(actor);

    StressAdjustmentResult result;
    result.before = actor.stress;
    result.beforeBand = bandForStress(actor.stress);

    actor.stress = std::clamp(actor.stress + delta, 0, actor.maxStress);
    result.after = actor.stress;
    result.afterBand = bandForStress(actor.stress);
    result.bandChanged = result.beforeBand != result.afterBand;
    result.applied = result.after - result.before;

    if (actor.stress < ResolveThreshold) {
        actor.resolveCheckTriggered = false;
    }

    if (delta > 0 && !actor.resolveCheckTriggered && actor.stress >= ResolveThreshold) {
        actor.resolveCheckTriggered = true;
        result.resolveCheckTriggered = true;

        if (random != nullptr && random->chance(PositiveResolveChance)) {
            removeTrait(actor, BreakdownTraitId);
            addTraitIfMissing(actor, ResolveTraitId);
            result.resolveOutcome = ResolveOutcome::Resolve;
        } else {
            removeTrait(actor, ResolveTraitId);
            addTraitIfMissing(actor, BreakdownTraitId);
            result.resolveOutcome = ResolveOutcome::Breakdown;
        }
    }

    result.collapsed = actor.stress >= actor.maxStress;
    return result;
}
} // namespace StressRules
