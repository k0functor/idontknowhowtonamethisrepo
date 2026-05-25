#include "RunMapGenerator.hpp"

RunMap RunMapGenerator::generateTestMap() const {
    RunMap map;

    map.nodes.push_back(RunMapNode{0, RunMapNodeType::Combat, RunMapNodeState::Available, Vector2{220.f, 520.f}, {1, 2}});
    map.nodes.push_back(RunMapNode{1, RunMapNodeType::Combat, RunMapNodeState::Locked, Vector2{430.f, 390.f}, {3}});
    map.nodes.push_back(RunMapNode{2, RunMapNodeType::Event, RunMapNodeState::Locked, Vector2{430.f, 650.f}, {3}});
    map.nodes.push_back(RunMapNode{3, RunMapNodeType::Rest, RunMapNodeState::Locked, Vector2{680.f, 520.f}, {4}});
    map.nodes.push_back(RunMapNode{4, RunMapNodeType::Boss, RunMapNodeState::Locked, Vector2{950.f, 520.f}, {}});

    map.currentNodeId = -1;
    return map;
}
