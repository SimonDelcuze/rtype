#pragma once

struct LivesComponent
{
    int current = 0;
    int max     = 0;

    static LivesComponent create(int currentLives, int maxLives);

    void loseLife(int amount = 1);
    void addLife(int amount = 1);
    void addExtraLife(int amount = 1);
    void setMax(int newMax);
    void resetToMax();
    bool isDead() const;
};
