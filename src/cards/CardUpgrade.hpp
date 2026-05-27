#pragma once

#include "cards/CardDefinition.hpp"

#include <optional>
#include <string>

namespace CardUpgrade {
bool isUpgradable(const CardDefinition& definition);
CardDefinition upgradedDefinition(const CardDefinition& definition);
CardDefinition effectiveDefinition(const CardDefinition& definition, bool upgraded);
std::string summary(const CardDefinition& base, const CardDefinition& upgraded);
}
