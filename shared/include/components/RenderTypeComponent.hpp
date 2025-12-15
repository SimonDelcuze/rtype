#pragma once

#include <cstdint>

struct RenderTypeComponent
{
    std::uint16_t typeId = 0;

    static RenderTypeComponent create(std::uint16_t type);
};

inline RenderTypeComponent RenderTypeComponent::create(std::uint16_t type)
{
    RenderTypeComponent c;
    c.typeId = type;
    return c;
}
