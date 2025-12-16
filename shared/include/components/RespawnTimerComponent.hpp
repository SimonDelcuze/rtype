#pragma once

struct RespawnTimerComponent
{
    float timeLeft = 0.0F;

    static RespawnTimerComponent create(float duration)
    {
        return {duration};
    }
};
