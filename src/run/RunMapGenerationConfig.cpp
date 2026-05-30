#include "RunMapGenerationConfig.hpp"

#include "data/JsonLoader.hpp"
#include "data/JsonReader.hpp"
#include "core/Random.hpp"

#include <stdexcept>
#include <string>

namespace {
RunMapSpecialNodeConfig parseSpecialNodeConfig(
    const Json& json,
    const RunMapSpecialNodeConfig& fallback,
    const std::filesystem::path& filePath,
    const std::string& name
) {
    if (!json.is_object()) {
        throw std::runtime_error(filePath.string() + ": '" + name + "' must be an object");
    }

    const JsonReader reader(json, filePath);
    RunMapSpecialNodeConfig result = fallback;
    result.count = reader.optionalInt("count", result.count);
    result.minLayer = reader.optionalInt("min_layer", result.minLayer);
    result.maxLayer = reader.optionalInt("max_layer", result.maxLayer);
    return result;
}

RunMapEliteConfig parseEliteConfig(
    const Json& json,
    const RunMapEliteConfig& fallback,
    const std::filesystem::path& filePath
) {
    if (!json.is_object()) {
        throw std::runtime_error(filePath.string() + ": 'elites' must be an object");
    }

    const JsonReader reader(json, filePath);
    RunMapEliteConfig result = fallback;
    result.minimum = reader.optionalInt("min", result.minimum);
    result.maximum = reader.optionalInt("max", result.maximum);
    result.minLayer = reader.optionalInt("min_layer", result.minLayer);
    result.maxLayer = reader.optionalInt("max_layer", result.maxLayer);
    return result;
}

RunMapEventConfig parseEventConfig(
    const Json& json,
    const RunMapEventConfig& fallback,
    const std::filesystem::path& filePath
) {
    if (!json.is_object()) {
        throw std::runtime_error(filePath.string() + ": 'events' must be an object");
    }

    const JsonReader reader(json, filePath);
    RunMapEventConfig result = fallback;
    result.minimum = reader.optionalInt("min", result.minimum);
    result.maximum = reader.optionalInt("max", result.maximum);
    result.minLayer = reader.optionalInt("min_layer", result.minLayer);
    result.maxLayer = reader.optionalInt("max_layer", result.maxLayer);
    return result;
}


std::vector<int> parseLayerNodeCounts(
    const Json& json,
    const std::filesystem::path& filePath
) {
    if (!json.is_array()) {
        throw std::runtime_error(filePath.string() + ": 'layer_node_counts' must be an array");
    }

    std::vector<int> result;
    result.reserve(json.size());

    for (std::size_t index = 0; index < json.size(); ++index) {
        const Json& value = json.at(index);
        if (!value.is_number_integer()) {
            throw std::runtime_error(filePath.string() + ": all elements of 'layer_node_counts' must be integers");
        }

        result.push_back(value.get<int>());
    }

    return result;
}

RunMapLayoutConfig parseLayoutConfig(
    const Json& json,
    const RunMapLayoutConfig& fallback,
    const std::filesystem::path& filePath
) {
    if (!json.is_object()) {
        throw std::runtime_error(filePath.string() + ": 'layout' must be an object");
    }

    const JsonReader reader(json, filePath);
    RunMapLayoutConfig result = fallback;
    result.startX = static_cast<float>(reader.optionalDouble("start_x", result.startX));
    result.layerStepX = static_cast<float>(reader.optionalDouble("layer_step_x", result.layerStepX));
    result.centerY = static_cast<float>(reader.optionalDouble("center_y", result.centerY));
    result.nodeSpacingY = static_cast<float>(reader.optionalDouble("node_spacing_y", result.nodeSpacingY));
    return result;
}
}

void RunMapGenerationConfig::loadFromFile(const std::filesystem::path& filePath) {
    const Json root = JsonLoader::loadObjectFromFile(filePath);
    const JsonReader reader(root, filePath);

    id_ = reader.optionalString("id", id_);
    layerCount_ = reader.optionalInt("layer_count", layerCount_);
    middleMinNodes_ = reader.optionalInt("middle_min_nodes", middleMinNodes_);
    middleMaxNodes_ = reader.optionalInt("middle_max_nodes", middleMaxNodes_);

    if (reader.has("layer_node_counts")) {
        layerNodeCounts_ = parseLayerNodeCounts(reader.requiredArray("layer_node_counts"), filePath);
        layerCount_ = static_cast<int>(layerNodeCounts_.size());
    }
    combatWeight_ = reader.optionalInt("combat_weight", combatWeight_);
    eventWeight_ = reader.optionalInt("event_weight", eventWeight_);
    extraConnectionChance_ = reader.optionalInt("extra_connection_chance", extraConnectionChance_);

    if (reader.has("shop")) {
        shop_ = parseSpecialNodeConfig(reader.requiredObject("shop"), shop_, filePath, "shop");
    }

    if (reader.has("chests")) {
        chests_ = parseSpecialNodeConfig(reader.requiredObject("chests"), chests_, filePath, "chests");
    }

    if (reader.has("elites")) {
        elites_ = parseEliteConfig(reader.requiredObject("elites"), elites_, filePath);
    }

    hasFixedEvents_ = reader.has("events");
    if (hasFixedEvents_) {
        events_ = parseEventConfig(reader.requiredObject("events"), events_, filePath);
    }

    if (reader.has("layout")) {
        layout_ = parseLayoutConfig(reader.requiredObject("layout"), layout_, filePath);
    }

    validate(filePath);
}

const std::string& RunMapGenerationConfig::id() const {
    return id_;
}

int RunMapGenerationConfig::layerCount() const {
    return layerCount_;
}

int RunMapGenerationConfig::middleMinNodes() const {
    return middleMinNodes_;
}

int RunMapGenerationConfig::middleMaxNodes() const {
    return middleMaxNodes_;
}

bool RunMapGenerationConfig::hasLayerNodeCounts() const {
    return !layerNodeCounts_.empty();
}

int RunMapGenerationConfig::nodeCountForLayer(const int layer, Random& random) const {
    if (hasLayerNodeCounts()) {
        if (layer < 0 || layer >= static_cast<int>(layerNodeCounts_.size())) {
            throw std::runtime_error("Requested run map layer is outside configured layer_node_counts");
        }

        return layerNodeCounts_[static_cast<std::size_t>(layer)];
    }

    return random.rangeInclusive(middleMinNodes_, middleMaxNodes_);
}

const std::vector<int>& RunMapGenerationConfig::layerNodeCounts() const {
    return layerNodeCounts_;
}

int RunMapGenerationConfig::combatWeight() const {
    return combatWeight_;
}

int RunMapGenerationConfig::eventWeight() const {
    return eventWeight_;
}

int RunMapGenerationConfig::extraConnectionChance() const {
    return extraConnectionChance_;
}

const RunMapSpecialNodeConfig& RunMapGenerationConfig::shop() const {
    return shop_;
}

const RunMapSpecialNodeConfig& RunMapGenerationConfig::chests() const {
    return chests_;
}

const RunMapEliteConfig& RunMapGenerationConfig::elites() const {
    return elites_;
}

const RunMapEventConfig& RunMapGenerationConfig::events() const {
    return events_;
}

bool RunMapGenerationConfig::hasFixedEvents() const {
    return hasFixedEvents_;
}

const RunMapLayoutConfig& RunMapGenerationConfig::layout() const {
    return layout_;
}

void RunMapGenerationConfig::validate(const std::filesystem::path& filePath) const {
    if (id_.empty()) {
        throw std::runtime_error(filePath.string() + ": act id must not be empty");
    }

    if (layerCount_ < 4) {
        throw std::runtime_error(filePath.string() + ": layer_count must be at least 4");
    }

    if (hasLayerNodeCounts()) {
        if (static_cast<int>(layerNodeCounts_.size()) != layerCount_) {
            throw std::runtime_error(filePath.string() + ": layer_node_counts size must match layer_count");
        }

        for (std::size_t index = 0; index < layerNodeCounts_.size(); ++index) {
            if (layerNodeCounts_[index] <= 0) {
                throw std::runtime_error(filePath.string() + ": layer_node_counts must contain only positive counts");
            }
        }
    }

    if (middleMinNodes_ <= 0 || middleMaxNodes_ <= 0 || middleMinNodes_ > middleMaxNodes_) {
        throw std::runtime_error(filePath.string() + ": middle node counts must be positive and min <= max");
    }

    if (combatWeight_ < 0 || eventWeight_ < 0 || combatWeight_ + eventWeight_ <= 0) {
        throw std::runtime_error(filePath.string() + ": combat/event weights must be non-negative and not both zero");
    }

    if (extraConnectionChance_ < 0 || extraConnectionChance_ > 100) {
        throw std::runtime_error(filePath.string() + ": extra_connection_chance must be between 0 and 100");
    }

    const int firstMiddleLayer = 1;
    const int lastMiddleLayer = layerCount_ - 3;

    if (lastMiddleLayer < firstMiddleLayer) {
        throw std::runtime_error(filePath.string() + ": layer_count leaves no middle layers");
    }

    if (shop_.count < 0) {
        throw std::runtime_error(filePath.string() + ": shop.count must not be negative");
    }

    if (shop_.minLayer < firstMiddleLayer || shop_.maxLayer > lastMiddleLayer || shop_.minLayer > shop_.maxLayer) {
        throw std::runtime_error(filePath.string() + ": shop layer range must be inside middle layers");
    }

    if (chests_.count < 0) {
        throw std::runtime_error(filePath.string() + ": chests.count must not be negative");
    }

    if (chests_.count > 0 && (chests_.minLayer < firstMiddleLayer || chests_.maxLayer > lastMiddleLayer || chests_.minLayer > chests_.maxLayer)) {
        throw std::runtime_error(filePath.string() + ": chests layer range must be inside middle layers");
    }

    if (elites_.minimum < 0 || elites_.maximum < 0 || elites_.minimum > elites_.maximum) {
        throw std::runtime_error(filePath.string() + ": elite min/max must be non-negative and min <= max");
    }

    if (elites_.minLayer < firstMiddleLayer || elites_.maxLayer > lastMiddleLayer || elites_.minLayer > elites_.maxLayer) {
        throw std::runtime_error(filePath.string() + ": elite layer range must be inside middle layers");
    }

    if (hasFixedEvents_) {
        if (events_.minimum < 0 || events_.maximum < 0 || events_.minimum > events_.maximum) {
            throw std::runtime_error(filePath.string() + ": event min/max must be non-negative and min <= max");
        }

        if (events_.minLayer < firstMiddleLayer || events_.maxLayer > lastMiddleLayer || events_.minLayer > events_.maxLayer) {
            throw std::runtime_error(filePath.string() + ": event layer range must be inside middle layers");
        }
    }

    const int guaranteedSpecials = shop_.count + chests_.count + elites_.maximum + (hasFixedEvents_ ? events_.maximum : 0);
    int middleCapacity = 0;
    if (hasLayerNodeCounts()) {
        for (int layer = firstMiddleLayer; layer <= lastMiddleLayer; ++layer) {
            middleCapacity += layerNodeCounts_[static_cast<std::size_t>(layer)];
        }
    } else {
        const int middleLayerCount = lastMiddleLayer - firstMiddleLayer + 1;
        middleCapacity = middleLayerCount * middleMaxNodes_;
    }

    if (guaranteedSpecials > middleCapacity) {
        throw std::runtime_error(filePath.string() + ": requested specials cannot fit into middle layers");
    }

    if (layout_.layerStepX <= 0.f || layout_.nodeSpacingY <= 0.f) {
        throw std::runtime_error(filePath.string() + ": layout spacing must be positive");
    }
}
