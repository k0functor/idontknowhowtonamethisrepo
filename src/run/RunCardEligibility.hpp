#pragma once

#include "cards/CardDefinition.hpp"
#include "run/RunState.hpp"

bool runCanReceiveCard(const RunState& run, const CardDefinition& card);
