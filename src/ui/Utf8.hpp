#pragma once

#include <SFML/System/String.hpp>

#include <string>

inline sf::String sfStringFromUtf8(const std::string& text) {
    return sf::String::fromUtf8(text.begin(), text.end());
}
