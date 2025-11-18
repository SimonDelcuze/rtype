#include "graphics/Window.hpp"

#include <optional>

Window::Window(const sf::VideoMode& mode, const std::string& title, bool vsync) : window_(mode, title, sf::Style::Close)
{
    window_.setVerticalSyncEnabled(vsync);
}

bool Window::isOpen() const
{
    return window_.isOpen();
}

void Window::close()
{
    window_.close();
}

void Window::pollEvents(const std::function<void(const sf::Event&)>& handler)
{
    while (const std::optional<sf::Event> event = window_.pollEvent()) {
        handler(*event);
    }
}

void Window::clear(const sf::Color& color)
{
    window_.clear(color);
}

void Window::display()
{
    window_.display();
}

void Window::draw(const sf::Drawable& drawable)
{
    window_.draw(drawable);
}

sf::RenderWindow& Window::raw()
{
    return window_;
}
