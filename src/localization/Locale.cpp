#include "Locale.hpp"

#include <stdexcept>
#include <utility>

Locale::Locale(std::string code) : code_(std::move(code)) {
    if (code_.empty()) {
        throw std::invalid_argument("Locale code cannot be empty");
    }
}

const std::string& Locale::code() const {
    return code_;
}

bool Locale::operator==(const Locale& other) const {
    return code_ == other.code_;
}

bool Locale::operator!=(const Locale& other) const {
    return !(*this == other);
}

Locale Locale::russian() {
    return Locale("ru");
}

Locale Locale::english() {
    return Locale("en");
}
