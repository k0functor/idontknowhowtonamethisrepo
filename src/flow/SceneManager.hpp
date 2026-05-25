#pragma once

#include "scenes/Scene.hpp"

#include <memory>

class SceneManager {
public:
    void setScene(std::unique_ptr<Scene> scene);

    void update(float deltaSeconds);
    void render() const;

    bool hasScene() const;

private:
    std::unique_ptr<Scene> scene_;
};
