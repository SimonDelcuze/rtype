#pragma once

#include <cstdint>
#include <unordered_map>

using EntityId = std::uint32_t;

struct CachedEntityState
{
    float posX       = 0.0F;
    float posY       = 0.0F;
    float velX       = 0.0F;
    float velY       = 0.0F;
    std::int16_t health = 0;
    std::int8_t lives   = 0;
    std::int32_t score  = 0;
    std::uint8_t status = 0;
    std::uint8_t entityType = 0;
    bool initialized = false;
};

class EntityStateCache
{
  public:
    CachedEntityState* get(EntityId id)
    {
        auto it = cache_.find(id);
        return it != cache_.end() ? &it->second : nullptr;
    }

    void update(EntityId id, const CachedEntityState& state)
    {
        cache_[id] = state;
    }

    void remove(EntityId id)
    {
        cache_.erase(id);
    }

    void clear()
    {
        cache_.clear();
    }

  private:
    std::unordered_map<EntityId, CachedEntityState> cache_;
};
