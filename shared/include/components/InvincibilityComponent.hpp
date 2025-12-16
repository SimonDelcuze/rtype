#pragma once

struct InvincibilityComponent
{
    float timeLeft   = 0.0F;
    bool isVisible   = true;
    float blinkTimer = 0.0F;

    static InvincibilityComponent create(float duration)
    {
        return {duration, true, 0.0F};
    }
};
