#include "ClientRuntime.hpp"
#include "network/ClientInit.hpp"
bool setupNetwork(NetPipelines& net, InputBuffer& inputBuffer, const IpEndpoint& serverEp,
                  std::atomic<bool>& handshakeDone, std::thread& welcomeThread,
                  ThreadSafeQueue<NotificationData>* broadcastQueue)
{
    if (!startReceiver(net, 0, handshakeDone, broadcastQueue))
        return false;
    if (!startSender(net, inputBuffer, 1, serverEp))
        return false;

    auto socketPtr = net.socket;
    welcomeThread  = std::thread([serverEp, &handshakeDone, socketPtr] {
        if (socketPtr)
            sendWelcomeLoop(serverEp, handshakeDone, *socketPtr);
    });
    return true;
}

void stopNetwork(NetPipelines& net, std::thread& welcomeThread, std::atomic<bool>& handshakeDone)
{
    handshakeDone.store(true);
    if (welcomeThread.joinable())
        welcomeThread.join();
    if (net.receiver)
        net.receiver->stop();
    if (net.sender)
        net.sender->stop();
}
