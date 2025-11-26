#include "components/ScoreComponent.hpp"

#include <algorithm>

ScoreComponent ScoreComponent::create(int initial)
{
    ScoreComponent s;
    s.value = initial;
    return s;
}

void ScoreComponent::add(int amount)
{
    if (amount <= 0) {
        return;
    }
    value += amount;
}

void ScoreComponent::subtract(int amount)
{
    if (amount <= 0) {
        return;
    }
    value = std::max(0, value - amount);
}

void ScoreComponent::reset()
{
    value = 0;
}

void ScoreComponent::set(int newValue)
{
    value = std::max(0, newValue);
}

bool ScoreComponent::isZero() const
{
    return value == 0;
}

bool ScoreComponent::isPositive() const
{
    return value > 0;
}
