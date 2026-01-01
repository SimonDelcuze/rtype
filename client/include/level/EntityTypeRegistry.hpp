#pragma once

#include "graphics/abstraction/ITexture.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

struct RenderTypeData
{
    std::shared_ptr<ITexture> texture = nullptr;
    std::uint8_t frameCount    = 1;
    float frameDuration        = 0.1F;
    std::uint32_t frameWidth   = 0;
    std::uint32_t frameHeight  = 0;
    std::uint32_t columns      = 1;
    std::uint8_t layer         = 0;
    const void* animation      = nullptr;
    std::string spriteId;
};

class EntityTypeRegistry
{
  public:
    void registerType(std::uint16_t typeId, RenderTypeData data);
    const RenderTypeData* get(std::uint16_t typeId) const;
    bool has(std::uint16_t typeId) const;
    void clear();
    std::size_t size() const;

  private:
    std::unordered_map<std::uint16_t, RenderTypeData> types_;
};
