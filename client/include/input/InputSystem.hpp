#pragma once

#include "input/InputBuffer.hpp"
#include "input/InputMapper.hpp"
#include "systems/ISystem.hpp"

class InputSystem : public ISystem
{
  public:
    InputSystem(InputBuffer& buffer, InputMapper& mapper, std::uint32_t& sequenceCounter, float& posX, float& posY);

    void initialize() override;
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override;

  private:
    InputCommand buildCommand(std::uint16_t flags, float angle);
    std::uint32_t nextSequence();

    InputBuffer* buffer_;
    InputMapper* mapper_;
    std::uint32_t* sequenceCounter_;
    float* posX_;
    float* posY_;
    bool positionInitialized_ = false;
    std::uint16_t lastFlags_   = 0;
    float fireCooldown_        = 0.5F;
    float fireElapsed_         = 0.0F;
};
