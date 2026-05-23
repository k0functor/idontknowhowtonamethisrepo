#pragma once

#include "Json.hpp"

#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>

class JsonReader {
public:
    JsonReader(const Json& json, std::filesystem::path sourcePath);

    bool has(const std::string& key) const;

    std::string requiredString(const std::string& key) const;
    std::int32_t requiredInt(const std::string& key) const;
    std::uint32_t requiredUnsigned(const std::string& key) const;
    bool requiredBool(const std::string& key) const;

    std::string optionalString(const std::string& key, const std::string& defaultValue) const;
    std::int32_t optionalInt(const std::string& key, std::int32_t defaultValue) const;
    std::uint32_t optionalUnsigned(const std::string& key, std::uint32_t defaultValue) const;
    bool optionalBool(const std::string& key, bool defaultValue) const;

    const Json& requiredObject(const std::string& key) const;
    const Json& optionalObject(const std::string& key) const;

    const Json& requiredArray(const std::string& key) const;
    const Json& optionalArray(const std::string& key) const;

    std::vector<std::string> optionalStringArray(
        const std::string& key,
        const std::vector<std::string>& defaultValue = {}
    ) const;

private:
    const Json& requireField(const std::string& key) const;

    [[noreturn]] void throwError(const std::string& message) const;

private:
    const Json& json_;
    std::filesystem::path sourcePath_;
};