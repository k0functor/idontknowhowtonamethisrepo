#pragma once

#include "data/ContentRegistry.hpp"

class LocalizationManager;

class ContentValidator {
public:
    static void validate(const ContentRegistry& content);
    static void validate(const ContentRegistry& content, const LocalizationManager& localization);
};
