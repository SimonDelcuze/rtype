#include "network/NetworkSender.hpp"

#include "Logger.hpp"

#include <chrono>

NetworkSender::NetworkSender(InputBuffer& buffer, IpEndpoint remote, std::uint32_t playerId,
                             std::chrono::milliseconds interval, IpEndpoint bind, ErrorHandler onError,
                             std::shared_ptr<UdpSocket> sharedSocket)
    : buffer_(&buffer), remote_(remote), bind_(bind), actualEndpoint_{}, socket_(std::move(sharedSocket)),
      ownsSocket_(false), thread_{}, stopRequested_{false}, running_{false}, interval_(interval), playerId_(playerId),
      onError_(std::move(onError))
{}

NetworkSender::~NetworkSender()
{
    stop();
}

bool NetworkSender::start()
{
    if (running_) {
        return false;
    }
    if (socket_ == nullptr) {
        socket_     = std::make_shared<UdpSocket>();
        ownsSocket_ = true;
    }
    if (!socket_->isOpen()) {
        if (!socket_->open(bind_)) {
            reportError(NetworkSendError("NetworkSender failed to open socket"));
            return false;
        }
    }
    socket_->setNonBlocking(true);
    stopRequested_  = false;
    running_        = true;
    actualEndpoint_ = socket_->localEndpoint();
    thread_         = std::thread(&NetworkSender::loop, this);
    return true;
}

void NetworkSender::stop()
{
    stopRequested_ = true;
    if (thread_.joinable()) {
        thread_.join();
    }
    if (ownsSocket_ && socket_ != nullptr) {
        socket_->close();
    }
    running_ = false;
}

bool NetworkSender::running() const
{
    return running_;
}

IpEndpoint NetworkSender::endpoint() const
{
    return actualEndpoint_;
}

void NetworkSender::loop()
{
    while (!stopRequested_) {
        auto startTick = std::chrono::steady_clock::now();
        auto cmdOpt    = buffer_->pop();
        if (cmdOpt) {
            sendCommand(*cmdOpt);
        }
        auto elapsed = std::chrono::steady_clock::now() - startTick;
        if (elapsed < interval_) {
            std::this_thread::sleep_for(interval_ - elapsed);
        }
    }
    running_ = false;
}

void NetworkSender::sendCommand(const InputCommand& cmd)
{
    auto packet = buildPacket(cmd);
    auto data   = packet.encode();
    auto res    = socket_->sendTo(data.data(), data.size(), remote_);
    if (!res.ok()) {
        reportError(NetworkSendSocketError("NetworkSender sendTo failed"));
    }
}

InputPacket NetworkSender::buildPacket(const InputCommand& cmd) const
{
    InputPacket packet{};
    packet.header.sequenceId = static_cast<std::uint16_t>(cmd.sequenceId & 0xFFFF);
    packet.playerId          = playerId_;
    packet.flags             = cmd.flags;
    packet.x                 = cmd.posX;
    packet.y                 = cmd.posY;
    packet.angle             = cmd.angle;
    return packet;
}

void NetworkSender::reportError(const IError& err)
{
    if (onError_) {
        onError_(err);
    }
}
