#include "Health.hpp"

#include <algorithm>
#include <stdexcept>

Health::Health(const int maximum)
    : Health(maximum, maximum) {}

Health::Health(const int current, const int maximum)
    : current_(current),
      maximum_(maximum) {
    if (maximum_ <= 0) {
        throw std::runtime_error("Health maximum must be positive");
    }

    current_ = std::clamp(current_, 0, maximum_);
}

int Health::current() const {
    return current_;
}

int Health::maximum() const {
    return maximum_;
}

void Health::setCurrent(const int value) {
    current_ = std::clamp(value, 0, maximum_);
}

void Health::setMaximum(const int value) {
    if (value <= 0) {
        throw std::runtime_error("Health maximum must be positive");
    }

    maximum_ = value;
    current_ = std::clamp(current_, 0, maximum_);
}

int Health::takeDamage(const int amount) {
    if (amount <= 0) {
        return 0;
    }

    const int before = current_;
    current_ = std::max(0, current_ - amount);
    return before - current_;
}

int Health::heal(const int amount) {
    if (amount <= 0) {
        return 0;
    }

    const int before = current_;
    current_ = std::min(maximum_, current_ + amount);
    return current_ - before;
}

bool Health::isDead() const {
    return current_ <= 0;
}
