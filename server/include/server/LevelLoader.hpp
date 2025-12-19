#pragma once

#include "server/LevelData.hpp"

#include <string>
#include <vector>

struct LevelRegistryEntry
{
    std::int32_t id = 0;
    std::string path;
    std::string name;
};

struct LevelRegistry
{
    std::int32_t schemaVersion = 1;
    std::vector<LevelRegistryEntry> levels;
};

enum class LevelLoadErrorCode
{
    FileNotFound,
    FileReadError,
    JsonParseError,
    SchemaError,
    SemanticError,
    RegistryError
};

struct LevelLoadError
{
    LevelLoadErrorCode code = LevelLoadErrorCode::FileNotFound;
    std::string message;
    std::string path;
    std::string jsonPointer;
};

class LevelLoader
{
  public:
    static bool load(std::int32_t levelId, LevelData& out, LevelLoadError& error);
    static bool loadFromPath(const std::string& path, LevelData& out, LevelLoadError& error);
    static bool loadRegistry(LevelRegistry& out, LevelLoadError& error);
    static std::string levelsRoot();
};
