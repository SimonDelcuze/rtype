#pragma once

#include <cstdint>

struct TypeComponent
{
    std::uint16_t typeId = 0;

    static TypeComponent create(std::uint16_t type)
    {
        TypeComponent c;
        c.typeId = type;
        return c;
    }
};
