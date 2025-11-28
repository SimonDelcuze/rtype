#pragma once

#include "errors/NetworkSendError.hpp"
#include "input/InputBuffer.hpp"
#include "network/InputPacket.hpp"
#include "network/UdpSocket.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

class NetworkSender
{
  public:
    using ErrorHandler = std::function<void(const IError&)>;

    NetworkSender(InputBuffer& buffer, IpEndpoint remote, std::uint32_t playerId,
                  std::chrono::milliseconds interval = std::chrono::milliseconds(16),
                  IpEndpoint bind = IpEndpoint::v4(0, 0, 0, 0, 0), ErrorHandler onError = nullptr);
    ~NetworkSender();

    bool start();
    void stop();
    bool running() const;
    IpEndpoint endpoint() const;

  private:
    void loop();
    void sendCommand(const InputCommand& cmd);
    InputPacket buildPacket(const InputCommand& cmd) const;
    void reportError(const IError& err);

    InputBuffer* buffer_;
    IpEndpoint remote_;
    IpEndpoint bind_;
    IpEndpoint actualEndpoint_;
    UdpSocket socket_;
    std::thread thread_;
    std::atomic<bool> stopRequested_{false};
    std::atomic<bool> running_{false};
    std::chrono::milliseconds interval_;
    std::uint32_t playerId_;
    ErrorHandler onError_;
};
