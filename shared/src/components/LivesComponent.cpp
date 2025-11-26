#include "components/LivesComponent.hpp"

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
    current -= amount;
    if (current < 0) {
        current = 0;
    }
}

void LivesComponent::addLife(int amount)
{
    if (amount <= 0) {
        return;
    }
    current += amount;
    if (current > max) {
        current = max;
    }
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
    if (newMax < 0) {
        newMax = 0;
    }
    max = newMax;
    if (current > max) {
        current = max;
    }
}

void LivesComponent::resetToMax()
{
    current = max;
}

bool LivesComponent::isDead() const
{
    return current <= 0;
}
