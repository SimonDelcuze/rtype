#pragma once

#include "input/KeyBindings.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
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
    };

    SettingsMenu(FontManager& fonts, TextureManager& textures, KeyBindings bindings = KeyBindings::defaults());

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const sf::Event& event) override;
    void render(Registry& registry, Window& window) override;
    Result getResult(Registry& registry) const;

  private:
    void startRebinding(Registry& registry, BindingAction action);
    void applyBinding(Registry& registry, BindingAction action, sf::Keyboard::Key key);
    void setAwaitingState(Registry& registry, BindingAction action, bool awaiting);
    void refreshButtonLabel(Registry& registry, BindingAction action);
    static std::string keyToString(sf::Keyboard::Key key);

    FontManager& fonts_;
    TextureManager& textures_;
    KeyBindings currentBindings_;
    bool done_ = false;
    std::optional<BindingAction> awaitingAction_;
    std::unordered_map<BindingAction, EntityId> actionButtons_;
};
