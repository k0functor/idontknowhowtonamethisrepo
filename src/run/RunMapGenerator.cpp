#include "RunMapGenerator.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct Slot {
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

bool containsId(const std::vector<int>& values, const int id) {
    return std::find(values.begin(), values.end(), id) != values.end();
}

void addEdge(RunMap& map, const int from, const int to) {
    std::vector<int>& next = nodeById(map, from).nextNodeIds;

    if (!containsId(next, to)) {
        next.push_back(to);
    }
}

void connectRandomly(
    RunMap& map,
    const std::vector<int>& fromLayer,
    const std::vector<int>& toLayer,
    Random& random
) {
    if (fromLayer.empty() || toLayer.empty()) {
        return;
    }

    std::vector<int> incomingCount(toLayer.size(), 0);

    for (const int from : fromLayer) {
        const int maximumEdges = std::min<int>(2, static_cast<int>(toLayer.size()));
        const int edgeCount = random.rangeInclusive(1, maximumEdges);

        for (int edge = 0; edge < edgeCount; ++edge) {
            const int targetIndex = random.rangeInclusive(0, static_cast<int>(toLayer.size()) - 1);
            const int targetId = toLayer[static_cast<std::size_t>(targetIndex)];
            addEdge(map, from, targetId);
            ++incomingCount[static_cast<std::size_t>(targetIndex)];
        }
    }

    // Every room in the next layer must be reachable from at least one previous room.
    for (std::size_t i = 0; i < toLayer.size(); ++i) {
        if (incomingCount[i] > 0) {
            continue;
        }

        const int from = fromLayer[static_cast<std::size_t>(
            random.rangeInclusive(0, static_cast<int>(fromLayer.size()) - 1)
        )];

        addEdge(map, from, toLayer[i]);
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

void validateGeneratedActOneMap(const RunMap& map, const int expectedShopCount, const int minimumEliteCount, const int maximumEliteCount) {
    int shopCount = 0;
    int eliteCount = 0;

    for (const RunMapNode& node : map.nodes) {
        if (node.type == RunMapNodeType::Shop) {
            ++shopCount;
        } else if (node.type == RunMapNodeType::Elite) {
            ++eliteCount;
        }
    }

    if (shopCount != expectedShopCount) {
        throw std::runtime_error("Generated act 1 map has invalid shop count");
    }

    if (eliteCount < minimumEliteCount || eliteCount > maximumEliteCount) {
        throw std::runtime_error("Generated act 1 map has invalid elite count");
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
    const int firstMiddleLayer = 1;
    const int lastMiddleLayer = layerCount - 3;

    std::vector<std::vector<RunMapNodeType>> layerTypes(static_cast<std::size_t>(layerCount));
    layerTypes[0] = {RunMapNodeType::Combat};
    layerTypes[static_cast<std::size_t>(preBossLayer)] = {RunMapNodeType::Rest};
    layerTypes[static_cast<std::size_t>(bossLayer)] = {RunMapNodeType::Boss};

    for (int layer = firstMiddleLayer; layer <= lastMiddleLayer; ++layer) {
        const int nodeCount = random.rangeInclusive(config.middleMinNodes(), config.middleMaxNodes());
        std::vector<RunMapNodeType>& types = layerTypes[static_cast<std::size_t>(layer)];
        types.reserve(static_cast<std::size_t>(nodeCount));

        for (int index = 0; index < nodeCount; ++index) {
            types.push_back(randomCombatOrEventRoomType(config, random));
        }
    }

    std::vector<Slot> usedSlots;

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

    nodeById(map, layerIds.front().front()).state = RunMapNodeState::Available;

    for (int layer = 0; layer < layerCount - 1; ++layer) {
        connectRandomly(
            map,
            layerIds[static_cast<std::size_t>(layer)],
            layerIds[static_cast<std::size_t>(layer + 1)],
            random
        );
    }

    map.currentNodeId = -1;
    validateGeneratedActOneMap(map, shop.count, elites.minimum, elites.maximum);
    return map;
}
