#pragma once

struct ScoreValueComponent
{
    int value = 0;

    static ScoreValueComponent create(int value)
    {
        ScoreValueComponent c;
        c.value = value < 0 ? 0 : value;
        return c;
    }
};
