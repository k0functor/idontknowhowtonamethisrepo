#pragma once

#include <string>
#include <string_view>

class Locale {
public:
    explicit Locale(std::string code);

    const std::string& code() const;

    bool operator==(const Locale& other) const;
    bool operator!=(const Locale& other) const;

    static Locale russian();
    static Locale english();

private:
    std::string code_;
};