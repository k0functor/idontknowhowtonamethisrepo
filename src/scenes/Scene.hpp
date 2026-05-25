#pragma once

class Scene {
public:
    virtual ~Scene() = default;

    virtual void update(float deltaSeconds) = 0;
    virtual void render() const = 0;
};
