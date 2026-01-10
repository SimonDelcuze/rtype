#pragma once

#include "ecs/Registry.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/Window.hpp"
#include "ui/IMenu.hpp"

#include <string>

class PasswordInputMenu : public IMenu
{
  public:
    struct Result
    {
        bool submitted = false;
        bool cancelled = false;
        std::string password;
    };

    PasswordInputMenu(FontManager& fonts, TextureManager& textures);

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const Event& event) override;
    void render(Registry& registry, Window& window) override;

    Result getResult(Registry& registry) const
    {
        (void) registry;
        return result_;
    }

  private:
    void onSubmit();
    void onCancel();

    FontManager& fonts_;
    TextureManager& textures_;
    Result result_;
    bool done_{false};

    EntityId backgroundEntity_{0};
    EntityId logoEntity_{0};
    EntityId titleEntity_{0};
    EntityId passwordLabelEntity_{0};
    EntityId passwordInputEntity_{0};
    EntityId submitButtonEntity_{0};
    EntityId cancelButtonEntity_{0};
};
