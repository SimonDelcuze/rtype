#include "components/InterpolationComponent.hpp"

void InterpolationComponent::setTarget(float x, float y)
{
    previousX = targetX;
    previousY = targetY;
    targetX   = x;
    targetY   = y;

    elapsedTime = 0.0F;
}

void InterpolationComponent::setTargetWithVelocity(float x, float y, float vx, float vy)
{
    setTarget(x, y);
    velocityX = vx;
    velocityY = vy;
}
