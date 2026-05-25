#include "SceneManager.hpp"

void SceneManager::setScene(std::unique_ptr<Scene> scene) {
    scene_ = std::move(scene);
}

void SceneManager::update(const float deltaSeconds) {
    if (scene_ != nullptr) {
        scene_->update(deltaSeconds);
    }
}

void SceneManager::render() const {
    if (scene_ != nullptr) {
        scene_->render();
    }
}

bool SceneManager::hasScene() const {
    return scene_ != nullptr;
}
