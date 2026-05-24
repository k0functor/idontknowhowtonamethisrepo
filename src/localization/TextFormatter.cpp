#include "TextFormatter.hpp"

#include <cstddef>

std::string TextFormatter::format(
    const std::string& templateText,
    const Variables& variables
) {
    std::string result = templateText;

    for(const auto& [name, value] : variables) {
        const std::string placeholder = "{" + name + "}";

        std::size_t position = 0;

        while((position = result.find(placeholder.position)) != std::string::npos) {
            result.replace(position.placeholder.length(), value);
            position += value.length();
        }
    }

    return result;
}