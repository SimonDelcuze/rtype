#pragma once

#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "input/KeyBindings.hpp"
#include "ui/IMenu.hpp"

#include <optional>
#include <unordered_map>

class SettingsMenu : public IMenu
{
  public:
    enum class BindingAction
    {
        Up,
        Down,
        Left,
        Right,
        Fire
    };

    struct Result
    {
        KeyBindings bindings;
        float musicVolume = 100.0F;
    };

    SettingsMenu(FontManager& fonts, TextureManager& textures, KeyBindings bindings = KeyBindings::defaults(),
                 float musicVolume = 100.0F);

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const Event& event) override;
    void render(Registry& registry, Window& window) override;
    Result getResult(Registry& registry) const;

  private:
    void startRebinding(Registry& registry, BindingAction action);
    void applyBinding(Registry& registry, BindingAction action, KeyCode key);
    void setAwaitingState(Registry& registry, BindingAction action, bool awaiting);
    void refreshButtonLabel(Registry& registry, BindingAction action);
    void setMusicVolume(Registry& registry, float volume);
    void refreshVolumeLabel(Registry& registry);
    bool handleVolumeMouseEvent(Registry& registry, const Vector2i& mousePos, bool isClick);
    void applyScrollOffset(Registry& registry);
    static std::string keyToString(KeyCode key);

    FontManager& fonts_;
    TextureManager& textures_;
    KeyBindings currentBindings_;
    bool done_ = false;
    std::optional<BindingAction> awaitingAction_;
    std::unordered_map<BindingAction, EntityId> actionButtons_;
    std::unordered_map<EntityId, float> originalPositions_;
    float musicVolume_           = 100.0F;
    EntityId volumeValueLabel_   = 0;
    EntityId networkDebugButton_ = 0;
    bool draggingVolume_         = false;
    float scrollOffset_          = 0.0F;
    float contentHeight_         = 0.0F;
    float baseSliderY_           = 0.0F;
    float sliderX_               = 520.0F;
    float sliderY_               = 0.0F;
    float sliderWidth_           = 320.0F;
    float sliderHeight_          = 10.0F;
};
