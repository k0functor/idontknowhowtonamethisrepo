#pragma once

class Health {
public:
    Health() = default;
    explicit Health(int maximum);
    Health(int current, int maximum);

    int current() const;
    int maximum() const;

    void setCurrent(int value);
    void setMaximum(int value);

    int takeDamage(int amount);
    int heal(int amount);

    bool isDead() const;

private:
    int current_ = 1;
    int maximum_ = 1;
};
