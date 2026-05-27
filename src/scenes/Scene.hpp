#pragma once

#include <string>
#include <vector>

class Scene {
public:
    virtual ~Scene() = default;

    virtual void update(float deltaSeconds) = 0;
    virtual void render() const = 0;

    virtual void onLocalizationChanged() {}

    virtual bool handleDebugCommand(const std::vector<std::string>&, std::string&) {
        return false;
    }
};
