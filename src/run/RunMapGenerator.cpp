#include "RunMapGenerator.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
struct Slot {
    int layer = 0;
    int index = 0;
};

struct NodeLocation {
    int layer = 0;
    int index = 0;
};

std::vector<float> offsetsForCount(const int count, const float spacing) {
    std::vector<float> offsets;
    offsets.reserve(static_cast<std::size_t>(std::max(0, count)));

    const float center = (static_cast<float>(count) - 1.f) * 0.5f;
    for (int i = 0; i < count; ++i) {
        offsets.push_back((static_cast<float>(i) - center) * spacing);
    }

    return offsets;
}

RunMapNodeType randomCombatOrEventRoomType(const RunMapGenerationConfig& config, Random& random) {
    const int totalWeight = config.combatWeight() + config.eventWeight();
    const int roll = random.rangeInclusive(1, totalWeight);

    if (roll <= config.combatWeight()) {
        return RunMapNodeType::Combat;
    }

    return RunMapNodeType::Event;
}

std::vector<int> addLayer(
    RunMap& map,
    const int layerIndex,
    const std::vector<RunMapNodeType>& types,
    int& nextId,
    const RunMapLayoutConfig& layout
) {
    std::vector<int> ids;
    ids.reserve(types.size());

    const std::vector<float> offsets = offsetsForCount(static_cast<int>(types.size()), layout.nodeSpacingY);
    const float x = layout.startX + static_cast<float>(layerIndex) * layout.layerStepX;

    for (std::size_t i = 0; i < types.size(); ++i) {
        RunMapNode node;
        node.id = nextId++;
        node.type = types[i];
        node.state = RunMapNodeState::Locked;
        node.position = Vector2{x, layout.centerY + offsets[i]};

        ids.push_back(node.id);
        map.nodes.push_back(std::move(node));
    }

    return ids;
}

RunMapNode& nodeById(RunMap& map, const int id) {
    for (RunMapNode& node : map.nodes) {
        if (node.id == id) {
            return node;
        }
    }

    throw std::runtime_error("Generated run map references unknown node id: " + std::to_string(id));
}

const RunMapNode& nodeById(const RunMap& map, const int id) {
    for (const RunMapNode& node : map.nodes) {
        if (node.id == id) {
            return node;
        }
    }

    throw std::runtime_error("Generated run map references unknown node id: " + std::to_string(id));
}

bool containsId(const std::vector<int>& values, const int id) {
    return std::find(values.begin(), values.end(), id) != values.end();
}

void addEdge(RunMap& map, const int from, const int to) {
    std::vector<int>& next = nodeById(map, from).nextNodeIds;

    if (!containsId(next, to)) {
        next.push_back(to);
    }
}

std::vector<int> adjacentTargetIndices(
    const int fromIndex,
    const int fromCount,
    const int toCount
) {
    std::vector<int> result;

    if (fromCount <= 0 || toCount <= 0) {
        return result;
    }

    if (fromCount == 1 || toCount == 1) {
        result.reserve(static_cast<std::size_t>(toCount));
        for (int index = 0; index < toCount; ++index) {
            result.push_back(index);
        }
        return result;
    }

    int first = (fromIndex * toCount) / fromCount;
    int last = ((fromIndex + 1) * toCount) / fromCount;

    if (last >= toCount) {
        last = toCount - 1;
    }

    if (first > last) {
        first = last;
    }

    for (int index = first; index <= last; ++index) {
        result.push_back(index);
    }

    return result;
}

bool isAllowedAdjacentTarget(
    const int fromIndex,
    const int fromCount,
    const int toIndex,
    const int toCount
) {
    const std::vector<int> allowed = adjacentTargetIndices(fromIndex, fromCount, toCount);
    return std::find(allowed.begin(), allowed.end(), toIndex) != allowed.end();
}

bool containsIndex(const std::vector<int>& values, const int candidate) {
    return std::find(values.begin(), values.end(), candidate) != values.end();
}

void addSelectedConnection(std::vector<std::vector<int>>& selectedTargetsBySource, const int sourceIndex, const int targetIndex) {
    std::vector<int>& targets = selectedTargetsBySource[static_cast<std::size_t>(sourceIndex)];
    if (!containsIndex(targets, targetIndex)) {
        targets.push_back(targetIndex);
    }
}

std::vector<int> sourceIndicesForTarget(
    const int targetIndex,
    const int fromCount,
    const int toCount
) {
    std::vector<int> result;
    for (int sourceIndex = 0; sourceIndex < fromCount; ++sourceIndex) {
        if (isAllowedAdjacentTarget(sourceIndex, fromCount, targetIndex, toCount)) {
            result.push_back(sourceIndex);
        }
    }
    return result;
}

void connectAdjacentLayers(
    RunMap& map,
    const std::vector<int>& fromLayer,
    const std::vector<int>& toLayer,
    Random& random,
    const int extraConnectionChance
) {
    const int fromCount = static_cast<int>(fromLayer.size());
    const int toCount = static_cast<int>(toLayer.size());

    if (fromCount == 0 || toCount == 0) {
        return;
    }

    if (fromCount == 1 || toCount == 1) {
        for (int fromIndex = 0; fromIndex < fromCount; ++fromIndex) {
            const std::vector<int> targets = adjacentTargetIndices(fromIndex, fromCount, toCount);
            for (const int targetIndex : targets) {
                addEdge(map, fromLayer[static_cast<std::size_t>(fromIndex)], toLayer[static_cast<std::size_t>(targetIndex)]);
            }
        }
        return;
    }

    std::vector<std::vector<int>> selectedTargetsBySource(static_cast<std::size_t>(fromCount));

    // First guarantee that every target in the next layer has at least one incoming edge.
    for (int targetIndex = 0; targetIndex < toCount; ++targetIndex) {
        const std::vector<int> sources = sourceIndicesForTarget(targetIndex, fromCount, toCount);
        if (sources.empty()) {
            throw std::runtime_error("Cannot connect generated run map: target has no adjacent source");
        }

        const int chosenSource = sources[static_cast<std::size_t>(random.rangeInclusive(0, static_cast<int>(sources.size()) - 1))];
        addSelectedConnection(selectedTargetsBySource, chosenSource, targetIndex);
    }

    // Then guarantee that every source room has at least one outgoing edge.
    for (int sourceIndex = 0; sourceIndex < fromCount; ++sourceIndex) {
        if (!selectedTargetsBySource[static_cast<std::size_t>(sourceIndex)].empty()) {
            continue;
        }

        const std::vector<int> targets = adjacentTargetIndices(sourceIndex, fromCount, toCount);
        if (targets.empty()) {
            throw std::runtime_error("Cannot connect generated run map: source has no adjacent target");
        }

        const int chosenTarget = targets[static_cast<std::size_t>(random.rangeInclusive(0, static_cast<int>(targets.size()) - 1))];
        addSelectedConnection(selectedTargetsBySource, sourceIndex, chosenTarget);
    }

    // Add a few extra adjacent edges randomly, so the map stays readable instead of fully connected.
    const double extraProbability = static_cast<double>(extraConnectionChance) / 100.0;
    int allowedEdgeCount = 0;
    for (int sourceIndex = 0; sourceIndex < fromCount; ++sourceIndex) {
        const std::vector<int> targets = adjacentTargetIndices(sourceIndex, fromCount, toCount);
        allowedEdgeCount += static_cast<int>(targets.size());
        for (const int targetIndex : targets) {
            if (!containsIndex(selectedTargetsBySource[static_cast<std::size_t>(sourceIndex)], targetIndex) &&
                random.chance(extraProbability)) {
                addSelectedConnection(selectedTargetsBySource, sourceIndex, targetIndex);
            }
        }
    }

    int selectedEdgeCount = 0;
    std::vector<int> incomingByTarget(static_cast<std::size_t>(toCount), 0);
    for (int sourceIndex = 0; sourceIndex < fromCount; ++sourceIndex) {
        const std::vector<int>& targets = selectedTargetsBySource[static_cast<std::size_t>(sourceIndex)];
        selectedEdgeCount += static_cast<int>(targets.size());
        for (const int targetIndex : targets) {
            ++incomingByTarget[static_cast<std::size_t>(targetIndex)];
        }
    }

    if (selectedEdgeCount >= allowedEdgeCount) {
        std::vector<std::pair<int, int>> removableEdges;
        for (int sourceIndex = 0; sourceIndex < fromCount; ++sourceIndex) {
            const std::vector<int>& targets = selectedTargetsBySource[static_cast<std::size_t>(sourceIndex)];
            if (targets.size() <= 1) {
                continue;
            }

            for (const int targetIndex : targets) {
                if (incomingByTarget[static_cast<std::size_t>(targetIndex)] > 1) {
                    removableEdges.emplace_back(sourceIndex, targetIndex);
                }
            }
        }

        if (!removableEdges.empty()) {
            const std::pair<int, int> edge = removableEdges[static_cast<std::size_t>(random.rangeInclusive(0, static_cast<int>(removableEdges.size()) - 1))];
            std::vector<int>& targets = selectedTargetsBySource[static_cast<std::size_t>(edge.first)];
            targets.erase(std::remove(targets.begin(), targets.end(), edge.second), targets.end());
        }
    }

    for (int sourceIndex = 0; sourceIndex < fromCount; ++sourceIndex) {
        std::vector<int>& targets = selectedTargetsBySource[static_cast<std::size_t>(sourceIndex)];
        std::sort(targets.begin(), targets.end());
        for (const int targetIndex : targets) {
            addEdge(map, fromLayer[static_cast<std::size_t>(sourceIndex)], toLayer[static_cast<std::size_t>(targetIndex)]);
        }
    }
}

bool sameSlot(const Slot& lhs, const Slot& rhs) {
    return lhs.layer == rhs.layer && lhs.index == rhs.index;
}

bool containsSlot(const std::vector<Slot>& slots, const Slot& candidate) {
    return std::any_of(slots.begin(), slots.end(), [&](const Slot& slot) {
        return sameSlot(slot, candidate);
    });
}

std::vector<Slot> collectSlots(
    const std::vector<std::vector<RunMapNodeType>>& layers,
    const int minLayer,
    const int maxLayer,
    const std::vector<Slot>& excluded
) {
    std::vector<Slot> result;

    const int clampedMin = std::max(0, minLayer);
    const int clampedMax = std::min(maxLayer, static_cast<int>(layers.size()) - 1);

    for (int layer = clampedMin; layer <= clampedMax; ++layer) {
        for (int index = 0; index < static_cast<int>(layers[static_cast<std::size_t>(layer)].size()); ++index) {
            Slot candidate{layer, index};
            if (!containsSlot(excluded, candidate)) {
                result.push_back(candidate);
            }
        }
    }

    return result;
}

void placeSpecials(
    std::vector<std::vector<RunMapNodeType>>& layers,
    std::vector<Slot>& usedSlots,
    std::vector<Slot> candidates,
    const int count,
    const RunMapNodeType type,
    Random& random,
    const std::string& label
) {
    if (count <= 0) {
        return;
    }

    if (count > static_cast<int>(candidates.size())) {
        throw std::runtime_error("Cannot place " + label + " nodes in generated act map: not enough candidate slots");
    }

    for (int i = 0; i < count; ++i) {
        const int index = random.rangeInclusive(0, static_cast<int>(candidates.size()) - 1);
        const Slot slot = candidates[static_cast<std::size_t>(index)];
        candidates.erase(candidates.begin() + index);

        layers[static_cast<std::size_t>(slot.layer)][static_cast<std::size_t>(slot.index)] = type;
        usedSlots.push_back(slot);
    }
}

int countNodesOfType(const RunMap& map, const RunMapNodeType type) {
    return static_cast<int>(std::count_if(map.nodes.begin(), map.nodes.end(), [&](const RunMapNode& node) {
        return node.type == type;
    }));
}

int countNodesOfTypeOnLayer(
    const RunMap& map,
    const std::vector<std::vector<int>>& layerIds,
    const RunMapNodeType type,
    const int layer
) {
    if (layer < 0 || layer >= static_cast<int>(layerIds.size())) {
        return 0;
    }

    return static_cast<int>(std::count_if(
        layerIds[static_cast<std::size_t>(layer)].begin(),
        layerIds[static_cast<std::size_t>(layer)].end(),
        [&](const int id) {
            return nodeById(map, id).type == type;
        }
    ));
}

std::unordered_map<int, NodeLocation> buildNodeLocations(const std::vector<std::vector<int>>& layerIds) {
    std::unordered_map<int, NodeLocation> result;

    for (int layer = 0; layer < static_cast<int>(layerIds.size()); ++layer) {
        for (int index = 0; index < static_cast<int>(layerIds[static_cast<std::size_t>(layer)].size()); ++index) {
            result[layerIds[static_cast<std::size_t>(layer)][static_cast<std::size_t>(index)]] = NodeLocation{layer, index};
        }
    }

    return result;
}

void validateGeneratedActOneMap(
    const RunMap& map,
    const std::vector<std::vector<int>>& layerIds,
    const RunMapGenerationConfig& config
) {
    const int shopCount = countNodesOfType(map, RunMapNodeType::Shop);
    const int chestCount = countNodesOfType(map, RunMapNodeType::Chest);
    const int eliteCount = countNodesOfType(map, RunMapNodeType::Elite);
    const int eventCount = countNodesOfType(map, RunMapNodeType::Event);

    if (shopCount != config.shop().count) {
        throw std::runtime_error("Generated act 1 map has invalid shop count");
    }

    if (chestCount != config.chests().count) {
        throw std::runtime_error("Generated act 1 map has invalid chest count");
    }

    if (config.chests().count > 0 && config.chests().minLayer == config.chests().maxLayer) {
        const int chestLayerCount = countNodesOfTypeOnLayer(map, layerIds, RunMapNodeType::Chest, config.chests().minLayer);
        if (chestLayerCount != config.chests().count) {
            throw std::runtime_error("Generated act 1 map has invalid chest layer placement");
        }
    }

    if (eliteCount < config.elites().minimum || eliteCount > config.elites().maximum) {
        throw std::runtime_error("Generated act 1 map has invalid elite count");
    }

    if (config.hasFixedEvents() && (eventCount < config.events().minimum || eventCount > config.events().maximum)) {
        throw std::runtime_error("Generated act 1 map has invalid event count");
    }

    if (config.hasLayerNodeCounts()) {
        for (int layer = 0; layer < static_cast<int>(layerIds.size()); ++layer) {
            if (static_cast<int>(layerIds[static_cast<std::size_t>(layer)].size()) != config.layerNodeCounts()[static_cast<std::size_t>(layer)]) {
                throw std::runtime_error("Generated act 1 map has invalid layer width");
            }
        }
    }

    const std::unordered_map<int, NodeLocation> locations = buildNodeLocations(layerIds);
    std::vector<int> incomingCount(map.nodes.size(), 0);

    for (const RunMapNode& node : map.nodes) {
        const auto fromIt = locations.find(node.id);
        if (fromIt == locations.end()) {
            throw std::runtime_error("Generated act 1 map contains a node outside layer index");
        }

        const NodeLocation from = fromIt->second;
        if (from.layer < static_cast<int>(layerIds.size()) - 1 && node.nextNodeIds.empty()) {
            throw std::runtime_error("Generated act 1 map contains a dead-end before boss");
        }

        for (const int toId : node.nextNodeIds) {
            const auto toIt = locations.find(toId);
            if (toIt == locations.end()) {
                throw std::runtime_error("Generated act 1 map references an unknown target node");
            }

            const NodeLocation to = toIt->second;
            if (to.layer != from.layer + 1) {
                throw std::runtime_error("Generated act 1 map contains a connection that skips layers");
            }

            const int fromCount = static_cast<int>(layerIds[static_cast<std::size_t>(from.layer)].size());
            const int toCount = static_cast<int>(layerIds[static_cast<std::size_t>(to.layer)].size());
            if (!isAllowedAdjacentTarget(from.index, fromCount, to.index, toCount)) {
                throw std::runtime_error("Generated act 1 map contains a non-adjacent connection");
            }

            if (toId >= 0 && toId < static_cast<int>(incomingCount.size())) {
                ++incomingCount[static_cast<std::size_t>(toId)];
            }
        }
    }

    for (int layer = 1; layer < static_cast<int>(layerIds.size()); ++layer) {
        for (const int id : layerIds[static_cast<std::size_t>(layer)]) {
            if (incomingCount[static_cast<std::size_t>(id)] <= 0) {
                throw std::runtime_error("Generated act 1 map contains an unreachable room");
            }
        }
    }
}
}

RunMap RunMapGenerator::generateTestMap() const {
    RunMapGenerationConfig config;
    Random random(1);
    return generateActOneMap(random, config);
}

RunMap RunMapGenerator::generateActOneMap(Random& random, const RunMapGenerationConfig& config) const {
    RunMap map;
    int nextId = 0;

    const int layerCount = config.layerCount();
    const int preBossLayer = layerCount - 2;
    const int bossLayer = layerCount - 1;
    std::vector<std::vector<RunMapNodeType>> layerTypes(static_cast<std::size_t>(layerCount));

    for (int layer = 0; layer < layerCount; ++layer) {
        const int nodeCount = (layer == 0 || layer == preBossLayer || layer == bossLayer)
            ? (config.hasLayerNodeCounts() ? config.nodeCountForLayer(layer, random) : 1)
            : config.nodeCountForLayer(layer, random);

        std::vector<RunMapNodeType>& types = layerTypes[static_cast<std::size_t>(layer)];
        types.reserve(static_cast<std::size_t>(nodeCount));

        for (int index = 0; index < nodeCount; ++index) {
            if (layer == bossLayer) {
                types.push_back(RunMapNodeType::Boss);
            } else if (layer == preBossLayer) {
                types.push_back(RunMapNodeType::Rest);
            } else if (layer == 0) {
                types.push_back(RunMapNodeType::Combat);
            } else {
                types.push_back(config.hasFixedEvents()
                    ? RunMapNodeType::Combat
                    : randomCombatOrEventRoomType(config, random));
            }
        }
    }

    std::vector<Slot> usedSlots;

    const RunMapSpecialNodeConfig& chests = config.chests();
    placeSpecials(
        layerTypes,
        usedSlots,
        collectSlots(layerTypes, chests.minLayer, chests.maxLayer, usedSlots),
        chests.count,
        RunMapNodeType::Chest,
        random,
        "chest"
    );

    const RunMapSpecialNodeConfig& shop = config.shop();
    placeSpecials(
        layerTypes,
        usedSlots,
        collectSlots(layerTypes, shop.minLayer, shop.maxLayer, usedSlots),
        shop.count,
        RunMapNodeType::Shop,
        random,
        "shop"
    );

    const RunMapEliteConfig& elites = config.elites();
    const int eliteCount = random.rangeInclusive(elites.minimum, elites.maximum);
    placeSpecials(
        layerTypes,
        usedSlots,
        collectSlots(layerTypes, elites.minLayer, elites.maxLayer, usedSlots),
        eliteCount,
        RunMapNodeType::Elite,
        random,
        "elite"
    );

    if (config.hasFixedEvents()) {
        const RunMapEventConfig& events = config.events();
        const int eventCount = random.rangeInclusive(events.minimum, events.maximum);
        placeSpecials(
            layerTypes,
            usedSlots,
            collectSlots(layerTypes, events.minLayer, events.maxLayer, usedSlots),
            eventCount,
            RunMapNodeType::Event,
            random,
            "event"
        );
    }

    std::vector<std::vector<int>> layerIds(static_cast<std::size_t>(layerCount));
    for (int layer = 0; layer < layerCount; ++layer) {
        layerIds[static_cast<std::size_t>(layer)] = addLayer(
            map,
            layer,
            layerTypes[static_cast<std::size_t>(layer)],
            nextId,
            config.layout()
        );
    }

    for (const int startId : layerIds.front()) {
        nodeById(map, startId).state = RunMapNodeState::Available;
    }

    for (int layer = 0; layer < layerCount - 1; ++layer) {
        connectAdjacentLayers(
            map,
            layerIds[static_cast<std::size_t>(layer)],
            layerIds[static_cast<std::size_t>(layer + 1)],
            random,
            config.extraConnectionChance()
        );
    }

    map.currentNodeId = -1;
    validateGeneratedActOneMap(map, layerIds, config);
    return map;
}
