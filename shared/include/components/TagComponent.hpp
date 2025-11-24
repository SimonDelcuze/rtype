#pragma once

#include <cstdint>

enum class EntityTag : std::uint8_t
{
    None       = 0,
    Player     = 1 << 0,
    Enemy      = 1 << 1,
    Projectile = 1 << 2,
    Pickup     = 1 << 3,
    Obstacle   = 1 << 4,
    Background = 1 << 5
};

inline EntityTag operator|(EntityTag a, EntityTag b)
{
    return static_cast<EntityTag>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}

inline EntityTag operator&(EntityTag a, EntityTag b)
{
    return static_cast<EntityTag>(static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b));
}

struct TagComponent
{
    EntityTag tags = EntityTag::None;

    static TagComponent create(EntityTag tags);
    bool hasTag(EntityTag tag) const;
    void addTag(EntityTag tag);
    void removeTag(EntityTag tag);
};

inline TagComponent TagComponent::create(EntityTag tags)
{
    TagComponent t;
    t.tags = tags;
    return t;
}

inline bool TagComponent::hasTag(EntityTag tag) const
{
    return (tags & tag) != EntityTag::None;
}

inline void TagComponent::addTag(EntityTag tag)
{
    tags = tags | tag;
}

inline void TagComponent::removeTag(EntityTag tag)
{
    tags = static_cast<EntityTag>(static_cast<std::uint8_t>(tags) & ~static_cast<std::uint8_t>(tag));
}
