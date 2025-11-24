#pragma once

#include <cstdint>

struct OwnershipComponent
{
    std::uint32_t ownerId = 0;
    std::uint8_t team     = 0;

    static OwnershipComponent create(std::uint32_t ownerId, std::uint8_t team = 0);
    bool isSameTeam(const OwnershipComponent& other) const;
    bool isSameOwner(const OwnershipComponent& other) const;
};

inline OwnershipComponent OwnershipComponent::create(std::uint32_t ownerId, std::uint8_t team)
{
    OwnershipComponent o;
    o.ownerId = ownerId;
    o.team    = team;
    return o;
}

inline bool OwnershipComponent::isSameTeam(const OwnershipComponent& other) const
{
    return team == other.team;
}

inline bool OwnershipComponent::isSameOwner(const OwnershipComponent& other) const
{
    return ownerId == other.ownerId;
}
