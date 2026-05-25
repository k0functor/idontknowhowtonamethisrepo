#pragma once

#include "cards/CardId.hpp"
#include "cards/CardInstanceId.hpp"

struct CardInstance {
    CardInstanceId instanceId;
    CardId definitionId;

    bool upgraded = false;
    bool temporary = false;
    bool markedForExhaust = false;
};
