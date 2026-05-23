#include "JsonReader.hpp"

#include <limits>
#include <stdexcept>

namespace {
    const Json& emptyObject() {
        static const Json object = Json::object();
        return object;
    }

    const Json& emptyArray() {
        static const Json array = Json::array();
        return array;
    }
}

JsonReader::JsonReader(const Json& json, std::filesystem::path sourcePath)
    : json_(json), 
      sourcePath_(std::move(sourcePath)) {
    if(!json_.is_object()) {
        throwError("Expected a JSON object");
    }
}

bool JsonReader::has(const std::string& key) const {
    return json_.contains(key);
}

std::string JsonReader::requiredString(const std::string& key) const {
    const Json& value = requireField(key);
    if(!value.is_string()) {
        throwError("Expected a string");
    }
    return value.get<std::string>();
}

std::int32_t JsonReader::requiredInt(const std::string& key) const {
    const Json& value = requireField(key);
    if(!value.is_number_integer()) {
        throwError("Expected an integer");
    }

    const std::int64_t number = value.get<std::int64_t>();

    if(number < std::numeric_limits<std::int32_t>::min() ||
       number > std::numeric_limits<std::int32_t>::max()) {
        throwError("'" + key + "' is out of range for std::int32_t");
    }
    
    return static_cast<std::int32_t>(number);
}

std::uint32_t JsonReader::requiredUnsigned(const std::string& key) const {
    const Json& value = requireField(key);
    if(!value.is_number_unsigned()) {
        throwError("Expected an unsigned integer");
    }

    const std::uint64_t number = value.get<std::uint64_t>();

    if(number > static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max())) {
        throwError("'" + key + "' is out of range for std::uint32_t");
    }
    
    return static_cast<std::uint32_t>(number);
}

bool JsonReader::requiredBool(const std::string& key) const {
    const Json& value = requireField(key);
    if(!value.is_boolean()) {
        throwError("Expected a boolean");
    }
    return value.get<bool>();
}

std::string JsonReader::optionalString(
    const std::string& key, 
    const std::string& defaultValue
) const {
    if(!json_.contains(key)) {
        return defaultValue;
    }
    return requiredString(key);
}

std::int32_t JsonReader::optionalInt(
    const std::string& key, 
    std::int32_t defaultValue
) const {
    if(!json_.contains(key)) {
        return defaultValue;
    }
    return requiredInt(key);
}

std::uint32_t JsonReader::optionalUnsigned(
    const std::string& key, 
    std::uint32_t defaultValue
) const {
    if(!json_.contains(key)) {
        return defaultValue;
    }
    return requiredUnsigned(key);
}

bool JsonReader::optionalBool(
    const std::string& key, 
    bool defaultValue
) const {
    if(!json_.contains(key)) {
        return defaultValue;
    }
    return requiredBool(key);
}

const Json& JsonReader::requiredObject(const std::string& key) const {
    const Json& value = requireField(key);
    if(!value.is_object()) {
        throwError("'" + key + "' must be an object");
    }
    return value;
}

const Json& JsonReader::optionalObject(const std::string& key) const {
    if(!json_.contains(key)) {
        return emptyObject();
    }
    return requiredObject(key);
}

const Json& JsonReader::requiredArray(const std::string& key) const {
    const Json& value = requireField(key);
    if(!value.is_array()) {
        throwError("'" + key + "' must be an array");
    }
    return value;
}

const Json& JsonReader::optionalArray(const std::string& key) const {
    if(!json_.contains(key)) {
        return emptyArray();
    }
    return requiredArray(key);
}

std::vector<std::string> JsonReader::optionalStringArray(
    const std::string& key,
    const std::vector<std::string>& defaultValue
) const {
    if(!has(key)) {
        return defaultValue;
    }

    const Json& array = requiredArray(key);

    std::vector<std::string> result;
    result.reserve(array.size());

    for(std::size_t index = 0; index < array.size(); ++index) {

        const Json& value = array.at(index);
        if(!value.is_string()) {
            throwError("Expected all elements of '" + key + "' to be strings (element " + std::to_string(index) + " is not)");
        }
        result.push_back(value.get<std::string>());
    }

    return result;
}

const Json& JsonReader::requireField(const std::string& key) const {
    if(!json_.contains(key)) {
        throwError("Missing required field '" + key + "'");
    }
    return json_.at(key);
}

[[noreturn]] void JsonReader::throwError(const std::string& message) const {
    throw std::runtime_error(sourcePath_.string() + ": " + message);
}
