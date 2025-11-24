#pragma once

struct BackgroundScrollComponent
{
    float speedX       = 0.0F;
    float speedY       = 0.0F;
    float resetOffsetX = 0.0F;
    float resetOffsetY = 0.0F;

    static BackgroundScrollComponent create(float sx, float sy, float resetX, float resetY = 0.0F)
    {
        BackgroundScrollComponent c;
        c.speedX       = sx;
        c.speedY       = sy;
        c.resetOffsetX = resetX;
        c.resetOffsetY = resetY;
        return c;
    }
};
