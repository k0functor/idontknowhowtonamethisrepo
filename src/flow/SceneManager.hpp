#pragma once

#include "scenes/Scene.hpp"

#include <memory>
#include <string>
#include <vector>

class SceneManager {
public:
    void setScene(std::unique_ptr<Scene> scene);

    void update(float deltaSeconds);
    void render() const;
    void notifyLocalizationChanged();
    bool handleDebugCommand(const std::vector<std::string>& tokens, std::string& output);

    bool hasScene() const;

private:
    std::unique_ptr<Scene> scene_;
};
