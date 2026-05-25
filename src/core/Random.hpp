#pragma once

#include <cstdint>
#include <random>

class Random {
public:
    Random();
    explicit Random(std::uint32_t seed);

    void setSeed(std::uint32_t seed);

    std::uint32_t seed() const;

    int rangeInclusive(int minimum, int maximum);
    bool chance(double probability);

private:
    std::uint32_t seed_ = 0;
    std::mt19937 engine_;
};
