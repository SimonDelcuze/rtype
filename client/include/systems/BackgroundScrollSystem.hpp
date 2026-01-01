#pragma once

#include "components/BackgroundScrollComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"
#include "graphics/Window.hpp"
#include "graphics/abstraction/ITexture.hpp"
#include "systems/ISystem.hpp"

class BackgroundScrollSystem : public ISystem
{
  public:
    explicit BackgroundScrollSystem(Window& window);
    void setNextBackground(const std::shared_ptr<ITexture>& texture);
    void update(Registry& registry, float deltaTime) override;

  private:
    Window& window_;
    std::shared_ptr<ITexture> nextTexture_ = nullptr;

    struct Entry
    {
        EntityId id;
        TransformComponent* transform;
        BackgroundScrollComponent* scroll;
        SpriteComponent* sprite;
        float width                       = 0.0F;
        float height                      = 0.0F;
        int layer                         = 0;
        std::shared_ptr<ITexture> texture = nullptr;
    };

    void collectEntries(Registry& registry, std::vector<Entry>& entries) const;
    void applyScaleAndOffsets(std::vector<Entry>& entries, float windowHeight) const;
    void moveAndTrack(std::vector<Entry>& entries, float deltaTime, float& minX, float& maxX) const;
    void ensureCoverage(Registry& registry, std::vector<Entry>& entries, float windowWidth) const;
    void wrapAndSwap(std::vector<Entry>& entries, float windowHeight);
    void applyTextureChange(Entry& entry, const std::shared_ptr<ITexture>& texture, float windowHeight) const;
};
