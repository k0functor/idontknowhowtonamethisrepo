#pragma once

#include "core/Random.hpp"
#include "run/StressRules.hpp"

#include <algorithm>
#include <optional>
#include <string_view>
#include <vector>

namespace StressBreakdownRules {
enum class BreakdownType {
    None,
    Discard,
    EnergyCrash,
    IntrusiveThoughts,
    CostSpike,
    Frenzy
};

inline int severityForStress(const int stress) {
    switch (StressRules::bandForStress(stress)) {
        case StressRules::StressBand::Panicked: return 1;
        case StressRules::StressBand::Breaking: return 2;
        case StressRules::StressBand::Calm:
        case StressRules::StressBand::Tense:
        case StressRules::StressBand::Pressured:
        case StressRules::StressBand::Collapsed:
            return 0;
    }
    return 0;
}

inline bool canTrigger(const int stress, const bool hasBreakdown, const bool hasResolve) {
    if (!hasBreakdown || hasResolve) {
        return false;
    }

    const StressRules::StressBand band = StressRules::bandForStress(stress);
    return band == StressRules::StressBand::Panicked || band == StressRules::StressBand::Breaking;
}

inline std::vector<BreakdownType> availableTypes(const int stress) {
    std::vector<BreakdownType> result{
        BreakdownType::Discard,
        BreakdownType::EnergyCrash,
        BreakdownType::IntrusiveThoughts
    };

    if (StressRules::bandForStress(stress) == StressRules::StressBand::Breaking) {
        result.push_back(BreakdownType::CostSpike);
        result.push_back(BreakdownType::Frenzy);
    }

    return result;
}


inline const char* toString(const BreakdownType type) {
    switch (type) {
        case BreakdownType::None: return "none";
        case BreakdownType::Discard: return "discard";
        case BreakdownType::EnergyCrash: return "energy";
        case BreakdownType::IntrusiveThoughts: return "status_cards";
        case BreakdownType::CostSpike: return "cost";
        case BreakdownType::Frenzy: return "frenzy";
    }
    return "none";
}

inline std::optional<BreakdownType> fromString(const std::string_view value) {
    if (value == "discard") return BreakdownType::Discard;
    if (value == "energy") return BreakdownType::EnergyCrash;
    if (value == "status_cards") return BreakdownType::IntrusiveThoughts;
    if (value == "cost") return BreakdownType::CostSpike;
    if (value == "frenzy") return BreakdownType::Frenzy;
    if (value == "none") return BreakdownType::None;
    return std::nullopt;
}

inline bool isAvailableAtStress(const BreakdownType type, const int stress) {
    const std::vector<BreakdownType> types = availableTypes(stress);
    return std::find(types.begin(), types.end(), type) != types.end();
}

inline BreakdownType choose(const int stress, Random& random) {
    const std::vector<BreakdownType> types = availableTypes(stress);
    if (types.empty()) {
        return BreakdownType::None;
    }

    const int index = random.rangeInclusive(0, static_cast<int>(types.size() - 1u));
    return types[static_cast<std::size_t>(index)];
}

inline const char* localizationSuffix(const BreakdownType type) {
    return toString(type);
}
} // namespace StressBreakdownRules
