#pragma once

#include <string>

struct FloorDefinition {
    std::string id;
    int index = 1;
    int act = 1;
    bool isImplemented = true;

    std::string nameTextId;
    std::string themeId;
    std::string mapConfigId;
    std::string mapConfigPath;
    std::string encounterTableId;
    std::string encounterTablePath;
    std::string eventPoolId;
    std::string nextFloorId;
};
