#pragma once

#include "ecs/ResetValue.hpp"

struct ChargeMeterComponent
{
    float progress = 0.0F;
};

template <> inline void resetValue<ChargeMeterComponent>(ChargeMeterComponent& value)
{
    value = ChargeMeterComponent{};
}
