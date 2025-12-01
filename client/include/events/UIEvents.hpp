#pragma once

#include <cstdint>
#include <string>

using EntityId = std::uint32_t;

struct ShowNotificationEvent
{
    std::string message;
    float duration = 3.0F;
    enum class Type
    {
        Info,
        Warning,
        Error,
        Success
    } type = Type::Info;
};

struct UpdateScoreDisplayEvent
{
    EntityId playerId;
    int score;
};

struct UpdateHealthDisplayEvent
{
    EntityId playerId;
    int currentHealth;
    int maxHealth;
};

struct UpdateLivesDisplayEvent
{
    EntityId playerId;
    int lives;
};

struct ShowDialogEvent
{
    std::string title;
    std::string message;
    bool pauseGame = false;
};

struct HideDialogEvent
{
};

struct UpdateAmmoDisplayEvent
{
    EntityId playerId;
    int currentAmmo;
    int maxAmmo;
};

struct ShowDamageIndicatorEvent
{
    float x;
    float y;
    int amount;
    bool isCritical = false;
};

struct ShowComboEvent
{
    int comboCount;
    float multiplier;
};

struct UpdateBossHealthBarEvent
{
    EntityId bossId;
    int currentHealth;
    int maxHealth;
    std::string bossName;
};

struct HideBossHealthBarEvent
{
};

struct ShowLevelTransitionEvent
{
    int fromLevel;
    int toLevel;
    float duration = 2.0F;
};

struct ShowPauseMenuEvent
{
};

struct HidePauseMenuEvent
{
};

struct ShowGameOverScreenEvent
{
    bool victory;
    int finalScore;
    int level;
};
