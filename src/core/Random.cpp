#include "Random.hpp"

#include <algorithm>
#include <random>
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
