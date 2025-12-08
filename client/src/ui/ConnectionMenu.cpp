#include "ui/ConnectionMenu.hpp"

#include "Logger.hpp"

#include <cstdlib>

ConnectionMenu::ConnectionMenu(FontManager& fonts)
    : done_(false), useDefault_(false), ipInput_("127.0.0.1"), portInput_("50010"), activeField_(ActiveField::None)
{
    if (fonts.has("ui")) {
        font_ = fonts.get("ui");
    } else {
        font_ = &fonts.load("ui", "client/assets/fonts/ui.ttf");
    }

    ipBox_.setSize(sf::Vector2f(400.0F, 50.0F));
    ipBox_.setPosition({440.0F, 290.0F});
    ipBox_.setFillColor(sf::Color(50, 50, 50));
    ipBox_.setOutlineColor(sf::Color(100, 100, 100));
    ipBox_.setOutlineThickness(2.0F);

    portBox_.setSize(sf::Vector2f(400.0F, 50.0F));
    portBox_.setPosition({440.0F, 430.0F});
    portBox_.setFillColor(sf::Color(50, 50, 50));
    portBox_.setOutlineColor(sf::Color(100, 100, 100));
    portBox_.setOutlineThickness(2.0F);

    connectButton_.setSize(sf::Vector2f(180.0F, 50.0F));
    connectButton_.setPosition({440.0F, 550.0F});
    connectButton_.setFillColor(sf::Color(0, 120, 200));
    connectButton_.setOutlineColor(sf::Color(0, 160, 240));
    connectButton_.setOutlineThickness(2.0F);

    defaultButton_.setSize(sf::Vector2f(180.0F, 50.0F));
    defaultButton_.setPosition({660.0F, 550.0F});
    defaultButton_.setFillColor(sf::Color(80, 80, 80));
    defaultButton_.setOutlineColor(sf::Color(120, 120, 120));
    defaultButton_.setOutlineThickness(2.0F);
}

void ConnectionMenu::handleEvent(const sf::Event& event)
{
    if (event.is<sf::Event::Closed>()) {
        done_ = true;
        return;
    }

    if (const auto* textEvent = event.getIf<sf::Event::TextEntered>()) {
        char c = static_cast<char>(textEvent->unicode);
        if (c >= 32 && c < 127) {
            handleTextInput(c);
        }
    }

    if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
        if (keyEvent->code == sf::Keyboard::Key::Backspace) {
            handleBackspace();
        } else if (keyEvent->code == sf::Keyboard::Key::Enter) {
            connectToServer();
        } else if (keyEvent->code == sf::Keyboard::Key::Tab) {
            if (activeField_ == ActiveField::IP) {
                activeField_ = ActiveField::Port;
            } else {
                activeField_ = ActiveField::IP;
            }
        }
    }

    if (const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mouseEvent->button == sf::Mouse::Button::Left) {
            handleMouseClick(mouseEvent->position.x, mouseEvent->position.y);
        }
    }
}

void ConnectionMenu::render(sf::RenderWindow& window)
{
    window.clear(sf::Color(30, 30, 40));

    sf::Text title(*font_, "R-Type - Server Connection", 48);
    title.setPosition({320.0F, 100.0F});
    title.setFillColor(sf::Color::White);
    window.draw(title);

    drawLabels(window);
    drawInputs(window);
    drawButtons(window);

    window.display();
}

void ConnectionMenu::drawLabels(sf::RenderWindow& window)
{
    sf::Text ipLabel(*font_, "Server IP:", 24);
    ipLabel.setPosition({440.0F, 250.0F});
    ipLabel.setFillColor(sf::Color(200, 200, 200));
    window.draw(ipLabel);

    sf::Text portLabel(*font_, "Port:", 24);
    portLabel.setPosition({440.0F, 390.0F});
    portLabel.setFillColor(sf::Color(200, 200, 200));
    window.draw(portLabel);
}

void ConnectionMenu::drawInputs(sf::RenderWindow& window)
{
    if (activeField_ == ActiveField::IP) {
        ipBox_.setOutlineColor(sf::Color(100, 200, 255));
    } else {
        ipBox_.setOutlineColor(sf::Color(100, 100, 100));
    }
    window.draw(ipBox_);

    if (activeField_ == ActiveField::Port) {
        portBox_.setOutlineColor(sf::Color(100, 200, 255));
    } else {
        portBox_.setOutlineColor(sf::Color(100, 100, 100));
    }
    window.draw(portBox_);

    sf::Text ipText(*font_, ipInput_ + (activeField_ == ActiveField::IP ? "_" : ""), 20);
    ipText.setPosition({450.0F, 303.0F});
    ipText.setFillColor(sf::Color::White);
    window.draw(ipText);

    sf::Text portText(*font_, portInput_ + (activeField_ == ActiveField::Port ? "_" : ""), 20);
    portText.setPosition({450.0F, 443.0F});
    portText.setFillColor(sf::Color::White);
    window.draw(portText);
}

void ConnectionMenu::drawButtons(sf::RenderWindow& window)
{
    window.draw(connectButton_);
    window.draw(defaultButton_);

    sf::Text connectLabel(*font_, "Connect", 22);
    connectLabel.setPosition({480.0F, 562.0F});
    connectLabel.setFillColor(sf::Color::White);
    window.draw(connectLabel);

    sf::Text defaultLabel(*font_, "Use Default", 22);
    defaultLabel.setPosition({675.0F, 562.0F});
    defaultLabel.setFillColor(sf::Color::White);
    window.draw(defaultLabel);
}

IpEndpoint ConnectionMenu::getServerEndpoint() const
{
    if (useDefault_) {
        return IpEndpoint::v4(127, 0, 0, 1, 50010);
    }

    std::uint16_t port = 50010;
    try {
        port = static_cast<std::uint16_t>(std::stoi(portInput_));
    } catch (...) {
        Logger::instance().warn("Invalid port, using 50010");
    }

    std::uint8_t a = 127, b = 0, c = 0, d = 1;
    if (sscanf(ipInput_.c_str(), "%hhu.%hhu.%hhu.%hhu", &a, &b, &c, &d) == 4) {
        return IpEndpoint::v4(a, b, c, d, port);
    }

    Logger::instance().warn("Invalid IP, using 127.0.0.1");
    return IpEndpoint::v4(127, 0, 0, 1, port);
}

void ConnectionMenu::handleTextInput(char c)
{
    if (activeField_ == ActiveField::IP) {
        if (ipInput_.length() < 15) {
            if (c == '.' || (c >= '0' && c <= '9')) {
                ipInput_ += c;
            }
        }
    } else if (activeField_ == ActiveField::Port) {
        if (portInput_.length() < 5) {
            if (c >= '0' && c <= '9') {
                portInput_ += c;
            }
        }
    }
}

void ConnectionMenu::handleBackspace()
{
    if (activeField_ == ActiveField::IP && !ipInput_.empty()) {
        ipInput_.pop_back();
    } else if (activeField_ == ActiveField::Port && !portInput_.empty()) {
        portInput_.pop_back();
    }
}

void ConnectionMenu::handleMouseClick(int x, int y)
{
    if (isPointInRect(x, y, ipBox_)) {
        activeField_ = ActiveField::IP;
    } else if (isPointInRect(x, y, portBox_)) {
        activeField_ = ActiveField::Port;
    } else if (isPointInRect(x, y, connectButton_)) {
        connectToServer();
    } else if (isPointInRect(x, y, defaultButton_)) {
        useDefaultServer();
    } else {
        activeField_ = ActiveField::None;
    }
}

void ConnectionMenu::connectToServer()
{
    Logger::instance().info("Connecting to " + ipInput_ + ":" + portInput_);
    done_       = true;
    useDefault_ = false;
}

void ConnectionMenu::useDefaultServer()
{
    Logger::instance().info("Using default server 127.0.0.1:50010");
    done_       = true;
    useDefault_ = true;
}

bool ConnectionMenu::isPointInRect(int x, int y, const sf::RectangleShape& rect) const
{
    auto pos  = rect.getPosition();
    auto size = rect.getSize();
    return x >= pos.x && x <= pos.x + size.x && y >= pos.y && y <= pos.y + size.y;
}
