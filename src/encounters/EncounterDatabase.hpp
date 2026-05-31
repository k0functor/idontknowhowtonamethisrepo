#pragma once

#include "core/Random.hpp"
#include "encounters/EncounterDefinition.hpp"
#include "run/RunMapNode.hpp"

#include <cstddef>
#include <filesystem>
#include <vector>

class EncounterDatabase {
public:
    void clear();
    void loadFromFile(const std::filesystem::path& filePath);

    const EncounterDefinition& choose(RunMapNodeType nodeType, Random& random, int layerIndex = -1) const;
    std::vector<const EncounterDefinition*> all() const;
    std::size_t size() const;

private:
    const std::vector<EncounterDefinition>& poolFor(RunMapNodeType nodeType) const;
    std::vector<EncounterDefinition>& mutablePoolFor(RunMapNodeType nodeType);

private:
    std::vector<EncounterDefinition> combat_;
    std::vector<EncounterDefinition> elite_;
    std::vector<EncounterDefinition> boss_;
};
