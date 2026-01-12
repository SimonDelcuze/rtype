#pragma once

#include "animation/AnimationRegistry.hpp"
#include "components/AudioComponent.hpp"
#include "components/InputHistoryComponent.hpp"
#include "graphics/TextureManager.hpp"
#include "input/InputBuffer.hpp"
#include "input/InputMapper.hpp"
#include "level/LevelState.hpp"
#include "systems/ISystem.hpp"

#include <optional>
#include <string>

class InputSystem : public ISystem
{
  public:
    InputSystem(std::uint32_t localPlayerId, InputBuffer& buffer, InputMapper& mapper, std::uint32_t& sequenceCounter,
                float& posX, float& posY, TextureManager& textures, AnimationRegistry& animations,
                LevelState* levelState = nullptr);
    InputSystem(std::uint32_t localPlayerId, InputBuffer& buffer, InputMapper& mapper, std::uint32_t& sequenceCounter,
                float& posX, float& posY, LevelState* levelState = nullptr);

    void initialize() override;
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override;

  private:
    InputCommand buildCommand(std::uint16_t flags, float angle, float deltaTime);
    std::uint32_t nextSequence();
    void sendChargedFireCommand(Registry& registry, float deltaTime);
    void ensureChargeFx(Registry& registry, float x, float y);
    void destroyChargeFx(Registry& registry);
    void updateChargeFx(Registry& registry, float x, float y);
    bool ensurePlayerPosition(Registry& registry);
    void updateChargeMeter(Registry& registry, float progress);
    void resetInputState(Registry& registry);
    void recordHistory(Registry& registry, const InputCommand& cmd, float deltaTime);
    bool hasLocalShield(Registry& registry) const;
    static TextureManager& dummyTextures();
    static AnimationRegistry& dummyAnimations();
    void startChargeSound(Registry& registry);
    void stopChargeSound(Registry& registry);
    void playChargedShotSound(Registry& registry);

    std::uint32_t localPlayerId_ = 0;
    InputBuffer* buffer_;
    InputMapper* mapper_;
    std::uint32_t* sequenceCounter_;
    float* posX_;
    float* posY_;
    bool positionInitialized_        = false;
    std::uint16_t lastSentMoveFlags_ = 0;
    TextureManager* textures_;
    AnimationRegistry* animations_;
    LevelState* levelState_ = nullptr;
    float fireElapsed_      = 0.0F;
    float repeatInterval_   = 0.02F;
    float repeatElapsed_    = 0.0F;
    float fireHoldTime_     = 0.0F;
    bool fireHeldLastFrame_ = false;
    std::optional<EntityId> chargeFxId_;
    std::optional<EntityId> playerId_;
    std::optional<EntityId> chargeMeterId_;
    std::optional<EntityId> chargedShotSoundId_;
    bool chargeSoundActive_          = false;
    const std::string chargeSoundId_ = "loading_sound";
    const std::string chargedShotId_ = "loading_laser";
    const float chargeFxDelay_       = 0.1F;
    const float maxChargeTime_       = 0.7F;
};
