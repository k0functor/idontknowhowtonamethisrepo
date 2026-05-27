#pragma once

#include "core/Random.hpp"

#include <algorithm>
#include <string>

namespace StressRules {
inline constexpr int ResolveThreshold = 100;
inline constexpr int MaximumStress = 200;
inline constexpr double PositiveResolveChance = 0.30;

inline constexpr const char* BreakdownTraitId = "stress_breakdown";
inline constexpr const char* ResolveTraitId = "stress_resolve";

enum class ResolveOutcome {
    None,
    Breakdown,
    Resolve
};

struct StressAdjustmentResult {
    int before = 0;
    int after = 0;
    int applied = 0;
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

    actor.stress = std::clamp(actor.stress + delta, 0, actor.maxStress);
    result.after = actor.stress;
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
