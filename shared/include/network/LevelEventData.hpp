#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

enum class LevelEventType : std::uint8_t
{
    SetScroll      = 1,
    SetBackground  = 2,
    SetMusic       = 3,
    SetCameraBounds = 4,
    GateOpen       = 5,
    GateClose      = 6
};

enum class LevelScrollMode : std::uint8_t
{
    Constant = 0,
    Stopped  = 1,
    Curve    = 2
};

struct LevelScrollKeyframe
{
    float time   = 0.0F;
    float speedX = 0.0F;
};

struct LevelScrollSettings
{
    LevelScrollMode mode = LevelScrollMode::Constant;
    float speedX         = 0.0F;
    std::vector<LevelScrollKeyframe> curve;
};

struct LevelCameraBounds
{
    float minX = 0.0F;
    float maxX = 0.0F;
    float minY = 0.0F;
    float maxY = 0.0F;
};

struct LevelEventData
{
    LevelEventType type = LevelEventType::SetScroll;
    std::optional<LevelScrollSettings> scroll;
    std::optional<std::string> backgroundId;
    std::optional<std::string> musicId;
    std::optional<LevelCameraBounds> cameraBounds;
    std::optional<std::string> gateId;
};
