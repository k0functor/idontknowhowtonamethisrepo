#pragma once

#include "run/RunMapNode.hpp"

#include <vector>

struct RunMap {
    std::vector<RunMapNode> nodes;
    int currentNodeId = -1;
};
