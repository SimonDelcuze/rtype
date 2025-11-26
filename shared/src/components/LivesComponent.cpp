#include "components/LivesComponent.hpp"

#include <algorithm>

LivesComponent LivesComponent::create(int currentLives, int maxLives)
{
    LivesComponent l;
    l.current = currentLives;
    l.max     = maxLives;
    return l;
}

void LivesComponent::loseLife(int amount)
{
    if (amount <= 0) {
        return;
    }
    current = std::max(0, current - amount);
}

void LivesComponent::addLife(int amount)
{
    if (amount <= 0) {
        return;
    }
    current = std::min(max, current + amount);
}

void LivesComponent::addExtraLife(int amount)
{
    if (amount <= 0) {
        return;
    }
    max += amount;
    current += amount;
}

void LivesComponent::setMax(int newMax)
{
    max     = std::max(0, newMax);
    current = std::min(current, max);
}

void LivesComponent::resetToMax()
{
    current = max;
}

bool LivesComponent::isDead() const
{
    return current <= 0;
}
