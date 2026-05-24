#pragma once

#include <string>
#include <unordered_map>

class TextFormatter {
public: 
    using Variables = std::unordered_map<std::string, std::string>;

    static std::string format(
        const std::string& templateText,
        const Variables& variables
    );
};