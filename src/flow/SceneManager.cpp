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

void SceneManager::notifyLocalizationChanged() {
    if (scene_ != nullptr) {
        scene_->onLocalizationChanged();
    }
}

bool SceneManager::handleDebugCommand(const std::vector<std::string>& tokens, std::string& output) {
    if (scene_ == nullptr) {
        return false;
    }

    return scene_->handleDebugCommand(tokens, output);
}

bool SceneManager::hasScene() const {
    return scene_ != nullptr;
}
