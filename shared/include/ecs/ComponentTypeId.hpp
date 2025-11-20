#pragma once

#include <atomic>
#include <cstddef>

class ComponentTypeId
{
  public:
    template <typename Component> static std::size_t value()
    {
        static const std::size_t id = next();
        return id;
    }

  private:
    static std::size_t next();
    static std::atomic<std::size_t> counter_;
};
