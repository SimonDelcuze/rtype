#pragma once

#include <string>

// Marks which spawn group (wave/obstacle/boss spawn) an entity belongs to.
struct SpawnGroupComponent
{
    std::string spawnId;

    static SpawnGroupComponent create(std::string id)
    {
        SpawnGroupComponent c;
        c.spawnId = std::move(id);
        return c;
    }
};
