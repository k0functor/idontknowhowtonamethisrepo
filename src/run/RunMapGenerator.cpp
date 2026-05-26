#include "RunMapGenerator.hpp"

#include <algorithm>
#include <vector>

namespace {
constexpr float START_X = 120.f;
constexpr float LAYER_STEP_X = 210.f;
constexpr float CENTER_Y = 520.f;

std::vector<float> offsetsForCount(const int count) {
    switch (count) {
        case 1:
            return {0.f};
        case 3:
            return {-190.f, 0.f, 190.f};
        case 4:
            return {-255.f, -85.f, 85.f, 255.f};
        default:
            return {0.f};
    }
}

RunMapNodeType randomRoomType(Random& random) {
    const int roll = random.rangeInclusive(1, 100);

    if (roll <= 62) {
        return RunMapNodeType::Combat;
    }

    if (roll <= 82) {
        return RunMapNodeType::Event;
    }

    return RunMapNodeType::Elite;
}

std::vector<int> addLayer(
    RunMap& map,
    const int layerIndex,
    const std::vector<RunMapNodeType>& types,
    int& nextId
) {
    std::vector<int> ids;
    ids.reserve(types.size());

    const std::vector<float> offsets = offsetsForCount(static_cast<int>(types.size()));
    const float x = START_X + static_cast<float>(layerIndex) * LAYER_STEP_X;

    for (std::size_t i = 0; i < types.size(); ++i) {
        RunMapNode node;
        node.id = nextId++;
        node.type = types[i];
        node.state = RunMapNodeState::Locked;
        node.position = Vector2{x, CENTER_Y + offsets[i]};

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

    // The generator owns all ids. This fallback is only here to keep release builds sane.
    return map.nodes.front();
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

void connectAllToAll(RunMap& map, const std::vector<int>& fromLayer, const std::vector<int>& toLayer) {
    for (const int from : fromLayer) {
        for (const int to : toLayer) {
            addEdge(map, from, to);
        }
    }
}

void connectAllToSingle(RunMap& map, const std::vector<int>& fromLayer, const int to) {
    for (const int from : fromLayer) {
        addEdge(map, from, to);
    }
}

void connectSingleToAll(RunMap& map, const int from, const std::vector<int>& toLayer) {
    for (const int to : toLayer) {
        addEdge(map, from, to);
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

    // Every visible room in the next layer must be reachable. Otherwise the map lies.
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

std::vector<RunMapNodeType> randomRooms(Random& random, const int count) {
    std::vector<RunMapNodeType> result;
    result.reserve(static_cast<std::size_t>(count));

    for (int i = 0; i < count; ++i) {
        result.push_back(randomRoomType(random));
    }

    return result;
}
}

RunMap RunMapGenerator::generateTestMap() const {
    Random random(1);
    return generateActOneMap(random);
}

RunMap RunMapGenerator::generateActOneMap(Random& random) const {
    RunMap map;
    int nextId = 0;

    const std::vector<int> layer0 = addLayer(map, 0, {RunMapNodeType::Combat}, nextId);
    const std::vector<int> layer1 = addLayer(map, 1, randomRooms(random, 3), nextId);
    const std::vector<int> layer2 = addLayer(map, 2, randomRooms(random, 4), nextId);
    const std::vector<int> layer3 = addLayer(map, 3, {RunMapNodeType::Chest}, nextId);
    const std::vector<int> layer4 = addLayer(map, 4, randomRooms(random, 3), nextId);
    const std::vector<int> layer5 = addLayer(map, 5, randomRooms(random, 3), nextId);
    const std::vector<int> layer6 = addLayer(map, 6, {RunMapNodeType::Rest}, nextId);
    const std::vector<int> layer7 = addLayer(map, 7, {RunMapNodeType::Boss}, nextId);

    nodeById(map, layer0.front()).state = RunMapNodeState::Available;

    connectSingleToAll(map, layer0.front(), layer1);
    connectRandomly(map, layer1, layer2, random);
    connectAllToSingle(map, layer2, layer3.front());
    connectSingleToAll(map, layer3.front(), layer4);
    connectRandomly(map, layer4, layer5, random);
    connectAllToSingle(map, layer5, layer6.front());
    connectAllToSingle(map, layer6, layer7.front());

    map.currentNodeId = -1;
    return map;
}
