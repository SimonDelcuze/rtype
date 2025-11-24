#pragma once

#include "components/HealthComponent.hpp"
#include "components/HitboxComponent.hpp"
#include "components/OwnershipComponent.hpp"
#include "components/TagComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/VelocityComponent.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace Serialization {

template <typename T> void write(std::vector<std::uint8_t>& buffer, const T& value)
{
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(&value);
    buffer.insert(buffer.end(), bytes, bytes + sizeof(T));
}

template <typename T> T read(const std::uint8_t* data, std::size_t& offset)
{
    T value;
    std::memcpy(&value, data + offset, sizeof(T));
    offset += sizeof(T);
    return value;
}

inline void serialize(std::vector<std::uint8_t>& buffer, const TransformComponent& t)
{
    write(buffer, t.x);
    write(buffer, t.y);
    write(buffer, t.rotation);
    write(buffer, t.scaleX);
    write(buffer, t.scaleY);
}

inline TransformComponent deserializeTransform(const std::uint8_t* data, std::size_t& offset)
{
    TransformComponent t;
    t.x        = read<float>(data, offset);
    t.y        = read<float>(data, offset);
    t.rotation = read<float>(data, offset);
    t.scaleX   = read<float>(data, offset);
    t.scaleY   = read<float>(data, offset);
    return t;
}

inline void serialize(std::vector<std::uint8_t>& buffer, const VelocityComponent& v)
{
    write(buffer, v.vx);
    write(buffer, v.vy);
}

inline VelocityComponent deserializeVelocity(const std::uint8_t* data, std::size_t& offset)
{
    VelocityComponent v;
    v.vx = read<float>(data, offset);
    v.vy = read<float>(data, offset);
    return v;
}

inline void serialize(std::vector<std::uint8_t>& buffer, const HitboxComponent& h)
{
    write(buffer, h.width);
    write(buffer, h.height);
    write(buffer, h.offsetX);
    write(buffer, h.offsetY);
    write(buffer, h.isActive);
}

inline HitboxComponent deserializeHitbox(const std::uint8_t* data, std::size_t& offset)
{
    HitboxComponent h;
    h.width    = read<float>(data, offset);
    h.height   = read<float>(data, offset);
    h.offsetX  = read<float>(data, offset);
    h.offsetY  = read<float>(data, offset);
    h.isActive = read<bool>(data, offset);
    return h;
}

inline void serialize(std::vector<std::uint8_t>& buffer, const HealthComponent& h)
{
    write(buffer, h.current);
    write(buffer, h.max);
}

inline HealthComponent deserializeHealth(const std::uint8_t* data, std::size_t& offset)
{
    HealthComponent h;
    h.current = read<std::int32_t>(data, offset);
    h.max     = read<std::int32_t>(data, offset);
    return h;
}

inline void serialize(std::vector<std::uint8_t>& buffer, const OwnershipComponent& o)
{
    write(buffer, o.ownerId);
    write(buffer, o.team);
}

inline OwnershipComponent deserializeOwnership(const std::uint8_t* data, std::size_t& offset)
{
    OwnershipComponent o;
    o.ownerId = read<std::uint32_t>(data, offset);
    o.team    = read<std::uint8_t>(data, offset);
    return o;
}

inline void serialize(std::vector<std::uint8_t>& buffer, const TagComponent& t)
{
    write(buffer, static_cast<std::uint8_t>(t.tags));
}

inline TagComponent deserializeTag(const std::uint8_t* data, std::size_t& offset)
{
    TagComponent t;
    t.tags = static_cast<EntityTag>(read<std::uint8_t>(data, offset));
    return t;
}

} // namespace Serialization
