#pragma once

#include "run/RunState.hpp"
#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"

#include <functional>
#include <optional>
#include <string>

#include <raylib.h>

class RunMapScene final : public Scene {
public:
    RunMapScene(
        const UiFont& font,
        const RunState& runState,
        std::function<void(int)> onNodeSelected,
        std::function<void(int)> onRestHeal,
        std::function<void(int)> onRestUpgrade,
        std::function<void()> onBackToHub
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    Vector2 nodeScreenPosition(const RunMapNode& node) const;
    Rectangle nodeBounds(const RunMapNode& node) const;
    Rectangle restModalBounds() const;
    Rectangle restHealButtonBounds(Rectangle modal) const;
    Rectangle restUpgradeButtonBounds(Rectangle modal) const;
    Rectangle restCancelButtonBounds(Rectangle modal) const;

    Color nodeColor(const RunMapNode& node) const;
    Color nodeOutlineColor(const RunMapNode& node) const;
    float nodeOutlineThickness(const RunMapNode& node) const;
    std::string nodeLabel(const RunMapNode& node) const;

    void updateRestModal(Vector2 mousePosition);
    void renderRestModal() const;

private:
    const UiFont& font_;
    const RunState& runState_;
    std::function<void(int)> onNodeSelected_;
    std::function<void(int)> onRestHeal_;
    std::function<void(int)> onRestUpgrade_;
    std::function<void()> onBackToHub_;

    std::optional<int> restModalNodeId_;
};
