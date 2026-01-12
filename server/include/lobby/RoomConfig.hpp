#pragma once

#include <algorithm>
#include <cstdint>

enum class RoomDifficulty : std::uint8_t
{
    Noob      = 0,
    Hell      = 1,
    Nightmare = 2,
    Custom    = 3
};

struct RoomConfig
{
    RoomDifficulty mode{RoomDifficulty::Noob};
    float enemyStatMultiplier{1.0F};
    float playerSpeedMultiplier{1.0F};
    float scoreMultiplier{1.0F};
    std::uint8_t playerLives{3};

    static RoomConfig preset(RoomDifficulty mode)
    {
        RoomConfig cfg{};
        cfg.mode = mode;
        switch (mode) {
            case RoomDifficulty::Noob:
                cfg.enemyStatMultiplier   = 0.5F;
                cfg.playerSpeedMultiplier = 1.0F;
                cfg.scoreMultiplier       = 0.5F;
                cfg.playerLives           = 3;
                break;
            case RoomDifficulty::Hell:
                cfg.enemyStatMultiplier   = 1.0F;
                cfg.playerSpeedMultiplier = 1.0F;
                cfg.scoreMultiplier       = 1.0F;
                cfg.playerLives           = 2;
                break;
            case RoomDifficulty::Nightmare:
                cfg.enemyStatMultiplier   = 1.5F;
                cfg.playerSpeedMultiplier = 0.67F;
                cfg.scoreMultiplier       = 1.5F;
                cfg.playerLives           = 1;
                break;
            case RoomDifficulty::Custom:
            default:
                break;
        }
        return cfg;
    }

    void clampCustom()
    {
        auto clamp = [](float v) { return std::clamp(v, 0.5F, 2.0F); };
        enemyStatMultiplier   = clamp(enemyStatMultiplier);
        playerSpeedMultiplier = clamp(playerSpeedMultiplier);
        scoreMultiplier       = clamp(scoreMultiplier);
        playerLives           = static_cast<std::uint8_t>(std::clamp<int>(playerLives, 1, 10));
    }
};
