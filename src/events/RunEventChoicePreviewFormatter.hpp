#pragma once

#include "consumables/ConsumableDatabase.hpp"
#include "data/CardDatabase.hpp"
#include "events/RunEventDefinition.hpp"
#include "events/RunEventRequirement.hpp"
#include "localization/LocalizationManager.hpp"
#include "relics/RelicDatabase.hpp"

#include <string>
#include <vector>

class RunEventChoicePreviewFormatter {
public:
    RunEventChoicePreviewFormatter(
        const LocalizationManager& localization,
        const CardDatabase& cards,
        const RelicDatabase& relics,
        const ConsumableDatabase& consumables
    );

    std::string describeChoice(const RunEventChoiceDefinition& choice) const;
    std::string describeEffects(const std::vector<RunEventEffect>& effects) const;
    std::string describeRequirements(const RunEventChoiceRequirements& requirements) const;
    std::string describeBlockReason(const RunEventChoiceBlockReason& reason) const;

private:
    std::string effectText(const RunEventEffect& effect) const;
    std::string cardName(const std::string& cardId) const;
    std::string relicName(const std::string& relicId) const;
    std::string consumableName(const std::string& consumableId) const;
    std::string traitName(const std::string& traitId) const;

private:
    const LocalizationManager& localization_;
    const CardDatabase& cards_;
    const RelicDatabase& relics_;
    const ConsumableDatabase& consumables_;
};
