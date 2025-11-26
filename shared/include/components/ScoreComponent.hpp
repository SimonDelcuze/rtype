#pragma once

struct ScoreComponent
{
    int value = 0;

    static ScoreComponent create(int initial);

    void add(int amount);
    void subtract(int amount);
    void reset();
    void set(int newValue);
    bool isZero() const;
    bool isPositive() const;
};
