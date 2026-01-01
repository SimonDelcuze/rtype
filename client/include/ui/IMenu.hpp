#pragma once

#include "ecs/Registry.hpp"
#include "graphics/Window.hpp"
#include "graphics/abstraction/Event.hpp"

class IMenu
{
  public:
    virtual ~IMenu() = default;

    virtual void create(Registry& registry)                          = 0;
    virtual void destroy(Registry& registry)                         = 0;
    virtual bool isDone() const                                      = 0;
    virtual void handleEvent(Registry& registry, const Event& event) = 0;
    virtual void render(Registry& registry, Window& window)          = 0;
};
