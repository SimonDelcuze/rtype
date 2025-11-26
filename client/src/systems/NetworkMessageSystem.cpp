#include "systems/NetworkMessageSystem.hpp"

NetworkMessageSystem::NetworkMessageSystem(NetworkMessageHandler& handler) : handler_(&handler) {}

void NetworkMessageSystem::initialize() {}

void NetworkMessageSystem::update(Registry&, float)
{
    handler_->poll();
}

void NetworkMessageSystem::cleanup() {}
