#include "input/InputSystem.hpp"

#include "ClientRuntime.hpp"
#include "Logger.hpp"
#include "components/AnimationComponent.hpp"
#include "components/AudioComponent.hpp"
#include "components/ChargeMeterComponent.hpp"
#include "components/HealthComponent.hpp"
#include "components/InputHistoryComponent.hpp"
#include "components/InvincibilityComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/LivesComponent.hpp"
#include "components/OwnershipComponent.hpp"
#include "components/RenderTypeComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TagComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/VelocityComponent.hpp"

#include <SFML/Graphics/Rect.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>

namespace
{
    constexpr float kMoveSpeed                  = 250.0F;
    constexpr std::uint16_t kShieldRenderTypeId = 25;
    constexpr float kShieldFeedbackDuration     = 1.5F;
} // namespace

InputSystem::InputSystem(std::uint32_t localPlayerId, InputBuffer& buffer, InputMapper& mapper,
                         std::uint32_t& sequenceCounter, float& posX, float& posY, TextureManager& textures,
                         AnimationRegistry& animations, LevelState* levelState)
    : localPlayerId_(localPlayerId), buffer_(&buffer), mapper_(&mapper), sequenceCounter_(&sequenceCounter),
      posX_(&posX), posY_(&posY), textures_(&textures), animations_(&animations), levelState_(levelState)
{}

InputSystem::InputSystem(std::uint32_t localPlayerId, InputBuffer& buffer, InputMapper& mapper,
                         std::uint32_t& sequenceCounter, float& posX, float& posY, LevelState* levelState)
    : InputSystem(localPlayerId, buffer, mapper, sequenceCounter, posX, posY, dummyTextures(), dummyAnimations(),
                  levelState)
{}

void InputSystem::initialize() {}

void InputSystem::update(Registry& registry, float deltaTime)
{
    ensurePlayerPosition(registry);
    if (levelState_ != nullptr && levelState_->introCinematicActive) {
        resetInputState(registry);
        return;
    }

    fireElapsed_ += deltaTime;
    repeatElapsed_ += deltaTime;

    auto flags                                       = mapper_->pollFlags();
    constexpr std::uint16_t kMovementAndInteractMask = InputMapper::UpFlag | InputMapper::DownFlag |
                                                       InputMapper::LeftFlag | InputMapper::RightFlag |
                                                       InputMapper::InteractFlag | InputMapper::BuyShieldFlag;
    std::uint16_t moves  = static_cast<std::uint16_t>(flags & kMovementAndInteractMask);
    bool changedMovement = moves != lastSentMoveFlags_;

    bool canFire = true;
    bool canMove = true;
    if (playerId_.has_value() && registry.isAlive(*playerId_)) {
        if (registry.has<TransformComponent>(*playerId_)) {
            if (registry.get<TransformComponent>(*playerId_).y < -5000.0F) {
                canMove = false;
                canFire = false;
            }
        }
        if (registry.has<HealthComponent>(*playerId_)) {
            if (registry.get<HealthComponent>(*playerId_).current <= 0) {
                canMove = false;
                canFire = false;
                resetInputState(registry);
            }
        }
        if (registry.has<InvincibilityComponent>(*playerId_))
            canFire = false;
        if (registry.has<LivesComponent>(*playerId_) && registry.get<LivesComponent>(*playerId_).current <= 0) {
            canFire = false;
            canMove = false;
        }
    }

    const bool firePressedRaw = (flags & InputMapper::FireFlag) != 0;
    const bool isDead         = !canMove || !canFire;

    if (isDead) {
        if (lastSentMoveFlags_ != 0 || fireHeldLastFrame_) {
            resetInputState(registry);
            InputCommand stopCmd = buildCommand(0, 0.0F, deltaTime);
            buffer_->push(stopCmd);
            recordHistory(registry, stopCmd, deltaTime);
            lastSentMoveFlags_ = 0;
            fireHeldLastFrame_ = false;
        }
        return;
    }

    const bool firePressed = firePressedRaw;

    if (firePressed) {
        fireHoldTime_ += deltaTime;
        if (fireHoldTime_ >= chargeFxDelay_) {
            ensureChargeFx(registry, *posX_, *posY_);
            updateChargeFx(registry, *posX_, *posY_);
            startChargeSound(registry);
        }
        float progress = std::clamp((fireHoldTime_ - chargeFxDelay_) / maxChargeTime_, 0.0F, 1.0F);
        updateChargeMeter(registry, progress);
    }
    if (!firePressed && fireHeldLastFrame_) {
        const bool wasCharging = fireHoldTime_ >= chargeFxDelay_;
        stopChargeSound(registry);
        destroyChargeFx(registry);
        if (wasCharging) {
            playChargedShotSound(registry);
        }
        sendChargedFireCommand(registry, deltaTime);
        fireHoldTime_ = 0.0F;
        updateChargeMeter(registry, 0.0F);
    }
    if (!firePressed && !fireHeldLastFrame_) {
        updateChargeMeter(registry, 0.0F);
        stopChargeSound(registry);
    }
    if (!canFire) {
        destroyChargeFx(registry);
        updateChargeMeter(registry, 0.0F);
        stopChargeSound(registry);
    }
    fireHeldLastFrame_ = firePressed;

    bool inactive = moves == 0;

    const bool left  = (moves & InputMapper::LeftFlag) != 0;
    const bool right = (moves & InputMapper::RightFlag) != 0;
    const bool up    = (moves & InputMapper::UpFlag) != 0;
    const bool down  = (moves & InputMapper::DownFlag) != 0;

    float dx = 0.0F;
    float dy = 0.0F;
    if (canMove) {
        if (left)
            dx -= 1.0F;
        if (right)
            dx += 1.0F;
        if (up)
            dy -= 1.0F;
        if (down)
            dy += 1.0F;
    }
    if (dx != 0.0F || dy != 0.0F) {
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.0F) {
            dx /= len;
            dy /= len;
        }
        const float stepX = dx * kMoveSpeed * deltaTime;
        const float stepY = dy * kMoveSpeed * deltaTime;
        *posX_ += stepX;
        *posY_ += stepY;
        if (playerId_.has_value() && registry.isAlive(*playerId_) && registry.has<TransformComponent>(*playerId_)) {
            auto& t = registry.get<TransformComponent>(*playerId_);
            t.x += stepX;
            t.y += stepY;
            if (registry.has<VelocityComponent>(*playerId_)) {
                auto& v = registry.get<VelocityComponent>(*playerId_);
                v.vx    = dx * kMoveSpeed;
                v.vy    = dy * kMoveSpeed;
            }
        }
    }

    float angle = 0.0F;
    if (dx != 0.0F || dy != 0.0F) {
        angle = std::atan2(dy, dx);
    }

    if (inactive && !changedMovement) {
        return;
    }

    const bool shouldSend = changedMovement || repeatElapsed_ >= repeatInterval_ || deltaTime == 0.0F;
    if (!shouldSend) {
        return;
    }

    InputCommand cmd = buildCommand(moves, angle, deltaTime);
    auto now         = std::chrono::steady_clock::now().time_since_epoch();
    cmd.captureTimestampNs =
        static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
    if ((cmd.flags & InputMapper::BuyShieldFlag) != 0) {
        if (levelState_ != nullptr && levelState_->safeZoneActive) {
            Logger::instance().info("[InputSystem] Sending BuyShield flag in input command");
            levelState_->shieldFeedback = hasLocalShield(registry) ? LevelState::ShieldFeedback::AlreadyActive
                                                                   : LevelState::ShieldFeedback::PurchaseRequested;
            levelState_->shieldFeedbackTimeRemaining = kShieldFeedbackDuration;
        } else {
            // Clear the flag if not in safe zone
            cmd.flags &= ~InputMapper::BuyShieldFlag;
        }
    }
    buffer_->push(cmd);
    recordHistory(registry, cmd, deltaTime);
    lastSentMoveFlags_ = moves;
    repeatElapsed_     = 0.0F;
}

void InputSystem::cleanup() {}

InputCommand InputSystem::buildCommand(std::uint16_t flags, float angle, float deltaTime)
{
    InputCommand cmd{};
    cmd.flags      = flags;
    cmd.sequenceId = nextSequence();
    cmd.deltaTime  = deltaTime;
    cmd.posX       = *posX_;
    cmd.posY       = *posY_;
    cmd.angle      = angle;
    return cmd;
}

std::uint32_t InputSystem::nextSequence()
{
    std::uint32_t seq = *sequenceCounter_;
    ++(*sequenceCounter_);
    return seq;
}

void InputSystem::sendChargedFireCommand(Registry& registry, float deltaTime)
{
    std::uint16_t flags = InputMapper::FireFlag;
    if (fireHoldTime_ < chargeFxDelay_) {
        InputCommand cmd = buildCommand(flags, 0.0F, deltaTime);
        auto now         = std::chrono::steady_clock::now().time_since_epoch();
        cmd.captureTimestampNs =
            static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
        buffer_->push(cmd);
        recordHistory(registry, cmd, deltaTime);
        return;
    }

    const float effective = std::max(0.0F, fireHoldTime_ - chargeFxDelay_);
    const float p         = std::clamp(effective / maxChargeTime_, 0.0F, 1.0F);
    if (p >= 1.0F) {
        flags |= InputMapper::Charge5Flag;
    } else if (p >= 0.6F) {
        flags |= InputMapper::Charge4Flag;
    } else if (p >= 0.4F) {
        flags |= InputMapper::Charge3Flag;
    } else {
        flags |= InputMapper::Charge2Flag;
    }

    InputCommand cmd = buildCommand(flags, 0.0F, deltaTime);
    auto now         = std::chrono::steady_clock::now().time_since_epoch();
    cmd.captureTimestampNs =
        static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
    buffer_->push(cmd);
    recordHistory(registry, cmd, deltaTime);
}

void InputSystem::ensureChargeFx(Registry& registry, float x, float y)
{
    if (chargeFxId_.has_value() && registry.isAlive(*chargeFxId_)) {
        return;
    }
    if (!textures_->has("bullet")) {
        try {
            textures_->load("bullet", "client/assets/sprites/bullet.png");
        } catch (...) {
        }
    }
    auto texPtr                         = textures_->get("bullet");
    std::shared_ptr<ITexture> actualTex = texPtr;
    const auto* clip                    = animations_->get("bullet_charge_warmup");

    if (!actualTex) {
        actualTex = textures_->getPlaceholder();
    }
    if (clip == nullptr || clip->frames.empty()) {
        return;
    }
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(x + 20.0F, y));
    auto& s = registry.emplace<SpriteComponent>(e, SpriteComponent(actualTex));
    s.customFrames.reserve(clip->frames.size());
    for (const auto& f : clip->frames) {
        s.customFrames.emplace_back(f.x, f.y, f.width, f.height);
    }
    s.setFrame(0);
    auto anim = AnimationComponent::create(static_cast<std::uint32_t>(clip->frames.size()), clip->frameTime, true);
    anim.loop = true;
    registry.emplace<AnimationComponent>(e, anim);
    registry.emplace<LayerComponent>(e, LayerComponent::create(RenderLayer::Effects));
    chargeFxId_ = e;
}

void InputSystem::destroyChargeFx(Registry& registry)
{
    if (chargeFxId_.has_value() && registry.isAlive(*chargeFxId_)) {
        registry.destroyEntity(*chargeFxId_);
    }
    chargeFxId_.reset();
}

void InputSystem::updateChargeFx(Registry& registry, float, float)
{
    if (!chargeFxId_.has_value() || !registry.isAlive(*chargeFxId_)) {
        chargeFxId_.reset();
        return;
    }
    ensurePlayerPosition(registry);
    auto& t = registry.get<TransformComponent>(*chargeFxId_);
    t.x     = *posX_ + 27.0F;
    t.y     = *posY_ - 6.0F;
}

TextureManager& InputSystem::dummyTextures()
{
    static TextureManager mgr;
    return mgr;
}

AnimationRegistry& InputSystem::dummyAnimations()
{
    static AnimationRegistry reg;
    return reg;
}

void InputSystem::updateChargeMeter(Registry& registry, float progress)
{
    if (!playerId_.has_value() || !registry.isAlive(*playerId_)) {
        return;
    }
    EntityId id = *playerId_;

    if (!registry.has<ChargeMeterComponent>(id)) {
        registry.emplace<ChargeMeterComponent>(id);
        if (!registry.has<TagComponent>(id)) {
            registry.emplace<TagComponent>(id, TagComponent::create(EntityTag::Player));
        }
    }
    auto& meter    = registry.get<ChargeMeterComponent>(id);
    meter.progress = progress;
}

void InputSystem::startChargeSound(Registry& registry)
{
    if (!playerId_.has_value() || !registry.isAlive(*playerId_)) {
        return;
    }
    const EntityId id = *playerId_;
    if (!registry.has<AudioComponent>(id)) {
        registry.emplace<AudioComponent>(id, AudioComponent::create(chargeSoundId_));
    }
    auto& audio   = registry.get<AudioComponent>(id);
    audio.soundId = chargeSoundId_;
    audio.loop    = true;
    audio.volume  = std::clamp(g_musicVolume, 0.0F, 100.0F);
    if (!chargeSoundActive_ || !audio.isPlaying) {
        audio.play();
    }
    chargeSoundActive_ = true;
}

void InputSystem::stopChargeSound(Registry& registry)
{
    if (!chargeSoundActive_) {
        return;
    }
    if (!playerId_.has_value() || !registry.isAlive(*playerId_) || !registry.has<AudioComponent>(*playerId_)) {
        chargeSoundActive_ = false;
        return;
    }
    auto& audio = registry.get<AudioComponent>(*playerId_);
    audio.loop  = false;
    audio.stop();
    chargeSoundActive_ = false;
}

void InputSystem::playChargedShotSound(Registry& registry)
{
    if (!chargedShotSoundId_.has_value() || !registry.isAlive(*chargedShotSoundId_)) {
        EntityId e = registry.createEntity();
        registry.emplace<AudioComponent>(e, AudioComponent::create(chargedShotId_));
        chargedShotSoundId_ = e;
    }
    if (!registry.has<AudioComponent>(*chargedShotSoundId_)) {
        registry.emplace<AudioComponent>(*chargedShotSoundId_, AudioComponent::create(chargedShotId_));
    }
    auto& audio  = registry.get<AudioComponent>(*chargedShotSoundId_);
    audio.loop   = false;
    audio.volume = std::clamp(g_musicVolume, 0.0F, 100.0F);
    audio.play(chargedShotId_);
}

void InputSystem::resetInputState(Registry& registry)
{
    destroyChargeFx(registry);
    updateChargeMeter(registry, 0.0F);
    stopChargeSound(registry);
    fireHoldTime_      = 0.0F;
    fireHeldLastFrame_ = false;
    fireElapsed_       = 0.0F;
    repeatElapsed_     = 0.0F;
    lastSentMoveFlags_ = 0;
}

bool InputSystem::hasLocalShield(Registry& registry) const
{
    for (EntityId entity : registry.view<RenderTypeComponent, OwnershipComponent>()) {
        if (!registry.isAlive(entity))
            continue;
        const auto& renderType = registry.get<RenderTypeComponent>(entity);
        if (renderType.typeId != kShieldRenderTypeId)
            continue;
        const auto& owner = registry.get<OwnershipComponent>(entity);
        if (owner.ownerId == localPlayerId_) {
            return true;
        }
    }
    return false;
}

bool InputSystem::ensurePlayerPosition(Registry& registry)
{
    if (playerId_.has_value() && registry.isAlive(*playerId_)) {
        const auto& t = registry.get<TransformComponent>(*playerId_);
        *posX_        = t.x;
        *posY_        = t.y;
        return true;
    }

    auto view = registry.view<TransformComponent, TagComponent>();
    for (auto id : view) {
        const auto& tag = registry.get<TagComponent>(id);
        if (!tag.hasTag(EntityTag::Player))
            continue;

        if (registry.has<OwnershipComponent>(id)) {
            const auto& ownership = registry.get<OwnershipComponent>(id);
            if (ownership.ownerId != localPlayerId_) {
                continue;
            }
        }

        const auto& t        = registry.get<TransformComponent>(id);
        *posX_               = t.x;
        *posY_               = t.y;
        playerId_            = id;
        positionInitialized_ = true;
        return true;
    }
    return false;
}

void InputSystem::recordHistory(Registry& registry, const InputCommand& cmd, float deltaTime)
{
    if (!playerId_.has_value() || !registry.isAlive(*playerId_)) {
        return;
    }
    EntityId id = *playerId_;
    if (!registry.has<InputHistoryComponent>(id)) {
        registry.emplace<InputHistoryComponent>(id);
    }
    auto& history = registry.get<InputHistoryComponent>(id);
    history.pushInput(cmd.sequenceId, cmd.flags, cmd.posX, cmd.posY, cmd.angle, deltaTime);
}
