#pragma once

#include "components/Components.hpp"
#include "ecs/Registry.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

enum class DirtyFlag : std::uint8_t
{
    None      = 0,
    Spawned   = 1 << 0,
    Destroyed = 1 << 1,
    Moved     = 1 << 2
};

inline DirtyFlag operator|(DirtyFlag a, DirtyFlag b)
{
    return static_cast<DirtyFlag>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}

inline bool hasFlag(DirtyFlag flags, DirtyFlag flag)
{
    return (static_cast<std::uint8_t>(flags) & static_cast<std::uint8_t>(flag)) != 0;
}

struct DirtyEntry
{
    EntityId id     = 0;
    DirtyFlag flags = DirtyFlag::None;
    TransformComponent transform{};
};

class DirtyTracker
{
  public:
    void onSpawn(EntityId id, const TransformComponent& t);
    void onDestroy(EntityId id);
    void trackTransform(EntityId id, const TransformComponent& t);
    std::vector<DirtyEntry> consume();

  private:
    static bool moved(const TransformComponent& a, const TransformComponent& b);

    std::unordered_map<EntityId, TransformComponent> previous_;
    std::unordered_map<EntityId, DirtyFlag> flags_;
};
