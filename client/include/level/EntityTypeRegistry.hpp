#pragma once

#include <SFML/Graphics/Texture.hpp>
#include <cstdint>
#include <unordered_map>

struct RenderTypeData
{
    const sf::Texture* texture = nullptr;
    std::uint8_t frameCount    = 1;
    float frameDuration        = 0.1F;
    std::uint8_t layer         = 0;
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
