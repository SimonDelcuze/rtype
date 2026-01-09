#include "graphics/Window.hpp"

#include "graphics/GraphicsFactory.hpp"

Window::Window(const Vector2u& size, const std::string& title)
{
    GraphicsFactory factory;
    window_ = factory.createWindow(size.x, size.y, title);
}

bool Window::isOpen() const
{
    return window_->isOpen();
}

Vector2u Window::getSize() const
{
    return window_->getSize();
}

void Window::close()
{
    window_->close();
}

void Window::pollEvents(const std::function<void(const Event&)>& handler)
{
    window_->pollEvents(handler);
}

void Window::clear(const Color& color)
{
    window_->clear(color);
}

void Window::display()
{
    window_->display();
}

void Window::draw(const ISprite& sprite)
{
    window_->draw(sprite);
}

void Window::draw(const IText& text)
{
    window_->draw(text);
}

void Window::draw(const Vector2f* vertices, std::size_t vertexCount, Color color, int type)
{
    window_->draw(vertices, vertexCount, color, type);
}

void Window::drawRectangle(Vector2f size, Vector2f position, float rotation, Vector2f scale, Color fillColor,
                           Color outlineColor, float outlineThickness)
{
    window_->drawRectangle(size, position, rotation, scale, fillColor, outlineColor, outlineThickness);
}

void Window::setColorFilter(ColorFilterMode mode)
{
    window_->setColorFilter(mode);
}

ColorFilterMode Window::getColorFilter() const
{
    return window_->getColorFilter();
}

std::shared_ptr<IWindow> Window::getNativeWindow() const
{
    return window_;
}
