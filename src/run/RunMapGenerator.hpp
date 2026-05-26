#pragma once

#include "core/Random.hpp"
#include "run/RunMap.hpp"

class RunMapGenerator {
public:
    RunMap generateTestMap() const;
    RunMap generateActOneMap(Random& random) const;
};
