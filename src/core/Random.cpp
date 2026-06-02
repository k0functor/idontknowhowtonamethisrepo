#include "Random.hpp"

#include <algorithm>
#include <random>
#include <sstream>
#include <stdexcept>

Random::Random()
    : Random(std::random_device{}()) {}

Random::Random(const std::uint32_t seed)
    : seed_(seed),
      engine_(seed) {}

void Random::setSeed(const std::uint32_t seed) {
    seed_ = seed;
    engine_.seed(seed_);
}

std::uint32_t Random::seed() const {
    return seed_;
}

std::string Random::state() const {
    std::ostringstream output;
    output << engine_;
    return output.str();
}

void Random::setState(const std::string& state) {
    std::istringstream input(state);
    input >> engine_;
    if (input.fail()) {
        throw std::runtime_error("Invalid Random state");
    }
}

int Random::rangeInclusive(int minimum, int maximum) {
    if (minimum > maximum) {
        throw std::runtime_error("Random::rangeInclusive called with minimum > maximum");
    }

    std::uniform_int_distribution<int> distribution(minimum, maximum);
    return distribution(engine_);
}

bool Random::chance(const double probability) {
    if (probability <= 0.0) {
        return false;
    }

    if (probability >= 1.0) {
        return true;
    }

    std::bernoulli_distribution distribution(probability);
    return distribution(engine_);
}
