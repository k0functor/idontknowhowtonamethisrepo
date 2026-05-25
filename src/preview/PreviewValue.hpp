#pragma once

#include <string>

struct PreviewValue {
    int minimum = 0;
    int maximum = 0;

    bool exact() const {
        return minimum == maximum;
    }

    std::string toDisplayString() const {
        if (exact()) {
            return std::to_string(minimum);
        }

        return std::to_string(minimum) + "-" + std::to_string(maximum);
    }
};
