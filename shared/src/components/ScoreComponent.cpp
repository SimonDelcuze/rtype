#include "components/ScoreComponent.hpp"

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
    value -= amount;
    if (value < 0) {
        value = 0;
    }
}

void ScoreComponent::reset()
{
    value = 0;
}

void ScoreComponent::set(int newValue)
{
    value = newValue < 0 ? 0 : newValue;
}

bool ScoreComponent::isZero() const
{
    return value == 0;
}

bool ScoreComponent::isPositive() const
{
    return value > 0;
}
