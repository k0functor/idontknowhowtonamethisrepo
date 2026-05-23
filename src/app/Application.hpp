#pragma once

#include "AppConfig.hpp"

#include <SFML/Graphics/RenderWindow.hpp>

class Application {
public:
    Application();

    int run();

private:
    void processEvents();
    void update();
    void render();

    void applyWindowSettings();
    
private:
    AppConfig config_;
    sf::RenderWindow window_;
};