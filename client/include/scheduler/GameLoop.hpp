#pragma once

#include "ecs/Registry.hpp"
#include "graphics/Window.hpp"
#include "scheduler/ClientScheduler.hpp"

class GameLoop : public ClientScheduler
{
  public:
    int run(Window& window, Registry& registry);
};
