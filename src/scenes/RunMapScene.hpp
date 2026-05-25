#pragma once

#include "run/RunState.hpp"
#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"

#include <functional>

class RunMapScene final : public Scene {
public:
    RunMapScene(
        const UiFont& font,
        const RunState& runState,
        std::function<void(int)> onNodeSelected,
        std::function<void()> onBackToHub
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    Rectangle nodeBounds(const RunMapNode& node) const;
    Color nodeColor(const RunMapNode& node) const;
    std::string nodeLabel(const RunMapNode& node) const;

private:
    const UiFont& font_;
    const RunState& runState_;
    std::function<void(int)> onNodeSelected_;
    std::function<void()> onBackToHub_;
};
