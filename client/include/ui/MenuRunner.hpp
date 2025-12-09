#pragma once

#include "ecs/Registry.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/Window.hpp"
#include "systems/ButtonSystem.hpp"
#include "systems/HUDSystem.hpp"
#include "systems/InputFieldSystem.hpp"
#include "ui/IMenu.hpp"

#include <atomic>
#include <memory>

class MenuRunner
{
  public:
    MenuRunner(Window& window, FontManager& fonts, TextureManager& textures, std::atomic<bool>& running);

    template <typename MenuType, typename... Args> void run(Args&&... args);

    template <typename MenuType, typename... Args> typename MenuType::Result runAndGetResult(Args&&... args);

    Registry& getRegistry();

  private:
    void runLoop(IMenu& menu);

    Window& window_;
    FontManager& fonts_;
    TextureManager& textures_;
    std::atomic<bool>& running_;
    Registry registry_;

    InputFieldSystem inputFieldSystem_;
    ButtonSystem buttonSystem_;
    HUDSystem hudSystem_;
};

template <typename MenuType, typename... Args> void MenuRunner::run(Args&&... args)
{
    MenuType menu(fonts_, textures_, std::forward<Args>(args)...);
    menu.create(registry_);
    runLoop(menu);
    menu.destroy(registry_);
}

template <typename MenuType, typename... Args> typename MenuType::Result MenuRunner::runAndGetResult(Args&&... args)
{
    MenuType menu(fonts_, textures_, std::forward<Args>(args)...);
    menu.create(registry_);
    runLoop(menu);
    auto result = menu.getResult(registry_);
    menu.destroy(registry_);
    return result;
}
