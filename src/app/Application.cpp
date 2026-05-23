#include "Application.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/VideoMode.hpp>

#include <optional>

namespace {
    sf::RenderWindow createWindow(const AppConfig& config) {
        const sf::State windowState = config.window.fullscreen
            ? sf::State::Fullscreen
            : sf::State::Windowed;

        const sf::VideoMode videoMode = config.window.fullscreen
            ? sf::VideoMode::getDesktopMode()
            : sf::VideoMode({config.window.width, config.window.height});

        return sf::RenderWindow(
            videoMode,
            config.window.title,
            sf::Style::Default,
            windowState
        );
    }
}

Application::Application() {
    config_ = AppConfig::loadFromFile("config.json");
    window_ = createWindow(config_);
    applyWindowSettings();
}

int Application::run() {
    while(window_.isOpen()) {
        processEvents();
        update();
        render();
    }

    return 0;
}

void Application::processEvents() {
    while(const std::optional event = window_.pollEvent()) {
        if(event->is<sf::Event::Closed>()) {
            window_.close();
        }
    }
}

void Application::update() {
    return;
}

void Application::render() {
    window_.clear(sf::Color(20, 20, 24));
    window_.display();
}

void Application::applyWindowSettings() {
    window_.setVerticalSyncEnabled(config_.window.verticalSync);

    if(!config_.window.verticalSync && config_.window.frameRateLimit > 0) {
        window_.setFramerateLimit(config_.window.frameRateLimit);
    }
}
