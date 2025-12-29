#include "input/InputSystem.hpp"

#include "Logger.hpp"
#include "components/AnimationComponent.hpp"
#include "components/ChargeMeterComponent.hpp"
#include "components/InvincibilityComponent.hpp"
#include "components/LivesComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TagComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/VelocityComponent.hpp"

#include <SFML/Graphics/Rect.hpp>
#include <chrono>
#include <cmath>

namespace
{
    constexpr float kMoveSpeed = 250.0F;
}

InputSystem::InputSystem(InputBuffer& buffer, InputMapper& mapper, std::uint32_t& sequenceCounter, float& posX,
                         float& posY, TextureManager& textures, AnimationRegistry& animations)
    : buffer_(&buffer), mapper_(&mapper), sequenceCounter_(&sequenceCounter), posX_(&posX), posY_(&posY),
      textures_(&textures), animations_(&animations)
{}

InputSystem::InputSystem(InputBuffer& buffer, InputMapper& mapper, std::uint32_t& sequenceCounter, float& posX,
                         float& posY)
    : InputSystem(buffer, mapper, sequenceCounter, posX, posY, dummyTextures(), dummyAnimations())
{}

void InputSystem::initialize() {}

void InputSystem::update(Registry& registry, float deltaTime)
{
    ensurePlayerPosition(registry);

    fireElapsed_ += deltaTime;
    repeatElapsed_ += deltaTime;

    auto flags           = mapper_->pollFlags();
    std::uint16_t moves  = static_cast<std::uint16_t>(flags & ~InputMapper::FireFlag);
    bool changedMovement = moves != lastSentMoveFlags_;

    bool canFire = true;
    if (playerId_.has_value() && registry.isAlive(*playerId_)) {
        if (registry.has<InvincibilityComponent>(*playerId_))
            canFire = false;
        if (registry.has<LivesComponent>(*playerId_) && registry.get<LivesComponent>(*playerId_).current <= 0)
            canFire = false;
    } else {
        canFire = false;
    }

    const bool firePressedRaw = (flags & InputMapper::FireFlag) != 0;
    const bool firePressed    = firePressedRaw && canFire;

    if (firePressed) {
        fireHoldTime_ += deltaTime;
        if (fireHoldTime_ >= chargeFxDelay_) {
            ensureChargeFx(registry, *posX_, *posY_);
            updateChargeFx(registry, *posX_, *posY_);
        }
        float progress = std::clamp((fireHoldTime_ - chargeFxDelay_) / maxChargeTime_, 0.0F, 1.0F);
        updateChargeMeter(registry, progress);
    }
    if (!firePressed && fireHeldLastFrame_) {
        destroyChargeFx(registry);
        sendChargedFireCommand();
        fireHoldTime_ = 0.0F;
        updateChargeMeter(registry, 0.0F);
    }
    if (!firePressed && !fireHeldLastFrame_) {
        updateChargeMeter(registry, 0.0F);
    }
    if (!canFire) {
        destroyChargeFx(registry);
        updateChargeMeter(registry, 0.0F);
    }
    fireHeldLastFrame_ = firePressed;

    bool inactive = moves == 0;

    const bool left  = (moves & InputMapper::LeftFlag) != 0;
    const bool right = (moves & InputMapper::RightFlag) != 0;
    const bool up    = (moves & InputMapper::UpFlag) != 0;
    const bool down  = (moves & InputMapper::DownFlag) != 0;

    float angle = 0.0F;

    float dx = 0.0F;
    float dy = 0.0F;
    if (left)
        dx -= 1.0F;
    if (right)
        dx += 1.0F;
    if (up)
        dy -= 1.0F;
    if (down)
        dy += 1.0F;
    if (dx != 0.0F || dy != 0.0F) {
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.0F) {
            dx /= len;
            dy /= len;
        }
        *posX_ += dx * kMoveSpeed * deltaTime;
        *posY_ += dy * kMoveSpeed * deltaTime;
    }

    if (inactive && !changedMovement) {
        return;
    }

    const bool shouldSend = changedMovement || repeatElapsed_ >= repeatInterval_ || deltaTime == 0.0F;
    if (!shouldSend) {
        return;
    }

    InputCommand cmd = buildCommand(moves, angle);
    auto now         = std::chrono::steady_clock::now().time_since_epoch();
    cmd.captureTimestampNs =
        static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
    buffer_->push(cmd);
    lastSentMoveFlags_ = moves;
    repeatElapsed_     = 0.0F;
}

void InputSystem::cleanup() {}

InputCommand InputSystem::buildCommand(std::uint16_t flags, float angle)
{
    InputCommand cmd{};
    cmd.flags      = flags;
    cmd.sequenceId = nextSequence();
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

void InputSystem::sendChargedFireCommand()
{
    std::uint16_t flags = InputMapper::FireFlag;
    if (fireHoldTime_ < chargeFxDelay_) {
        InputCommand cmd = buildCommand(flags, 0.0F);
        auto now         = std::chrono::steady_clock::now().time_since_epoch();
        cmd.captureTimestampNs =
            static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
        buffer_->push(cmd);
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

    InputCommand cmd = buildCommand(flags, 0.0F);
    auto now         = std::chrono::steady_clock::now().time_since_epoch();
    cmd.captureTimestampNs =
        static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
    buffer_->push(cmd);
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
    const auto* tex  = textures_->get("bullet");
    const auto* clip = animations_->get("bullet_charge_warmup");
    if (tex == nullptr) {
        tex = &textures_->getPlaceholder();
    }
    if (clip == nullptr || clip->frames.empty()) {
        return;
    }
    EntityId e = registry.createEntity();
    registry.emplace<TransformComponent>(e, TransformComponent::create(x + 20.0F, y));
    auto& s = registry.emplace<SpriteComponent>(e, SpriteComponent(*tex));
    s.customFrames.reserve(clip->frames.size());
    for (const auto& f : clip->frames) {
        s.customFrames.emplace_back(sf::Vector2i{f.x, f.y}, sf::Vector2i{f.width, f.height});
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
    EntityId id{};
    if (chargeMeterId_.has_value() && registry.isAlive(*chargeMeterId_)) {
        id = *chargeMeterId_;
    } else {
        id             = registry.createEntity();
        chargeMeterId_ = id;
    }
    if (!registry.has<ChargeMeterComponent>(id)) {
        registry.emplace<ChargeMeterComponent>(id);
    }
    auto& meter    = registry.get<ChargeMeterComponent>(id);
    meter.progress = progress;
}

bool InputSystem::ensurePlayerPosition(Registry& registry)
{
    auto view = registry.view<TransformComponent, TagComponent>();
    for (auto id : view) {
        const auto& tag = registry.get<TagComponent>(id);
        if (!tag.hasTag(EntityTag::Player))
            continue;
        const auto& t        = registry.get<TransformComponent>(id);
        *posX_               = t.x;
        *posY_               = t.y;
        playerId_            = id;
        positionInitialized_ = true;
        return true;
    }
    return false;
}
