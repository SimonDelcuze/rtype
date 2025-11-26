#pragma once

#include "network/NetworkMessageHandler.hpp"
#include "systems/ISystem.hpp"

class NetworkMessageSystem : public ISystem
{
  public:
    explicit NetworkMessageSystem(NetworkMessageHandler& handler);
    void initialize() override;
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override;

  private:
    NetworkMessageHandler* handler_;
};
