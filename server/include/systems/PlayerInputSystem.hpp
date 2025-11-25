#pragma once

#include "components/Components.hpp"
#include "network/InputReceiveThread.hpp"

class PlayerInputSystem
{
  public:
    explicit PlayerInputSystem(float speed = 1.0F);
    void update(Registry& registry, const std::vector<ReceivedInput>& inputs) const;

  private:
    float speed_;
};
