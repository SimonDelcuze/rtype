#pragma once

struct ScoreComponent
{
    int value = 0;

    static ScoreComponent create(int initial)
    {
        ScoreComponent s;
        s.value = initial;
        return s;
    }
};
