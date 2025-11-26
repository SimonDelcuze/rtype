#pragma once

struct LivesComponent
{
    int current = 0;
    int max     = 0;

    static LivesComponent create(int currentLives, int maxLives)
    {
        LivesComponent l;
        l.current = currentLives;
        l.max     = maxLives;
        return l;
    }
};
