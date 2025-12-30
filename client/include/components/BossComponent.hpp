#pragma once

#include <string>

struct BossComponent
{
    std::string name;

    static BossComponent create(const std::string& name = "Boss")
    {
        BossComponent c;
        c.name = name;
        return c;
    }
};
