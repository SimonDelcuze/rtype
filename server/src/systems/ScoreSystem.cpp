#include "systems/ScoreSystem.hpp"

#include "components/OwnershipComponent.hpp"

ScoreSystem::ScoreSystem(EventBus& bus, Registry& registry) : registry_(&registry)
{
    bus.subscribe<DamageEvent>([this](const DamageEvent& event) { this->onDamage(event); });
}

void ScoreSystem::onDamage(const DamageEvent& event)
{
    if (registry_ == nullptr) {
        return;
    }
    if (event.amount <= 0 || event.remaining > 0) {
        return;
    }
    if (!registry_->isAlive(event.target) || !registry_->has<ScoreValueComponent>(event.target)) {
        return;
    }
    if (!registry_->has<TagComponent>(event.target) ||
        !registry_->get<TagComponent>(event.target).hasTag(EntityTag::Enemy)) {
        return;
    }
    if (!registry_->isAlive(event.attacker) || !registry_->has<TagComponent>(event.attacker)) {
        return;
    }

    const auto& tag = registry_->get<TagComponent>(event.attacker);

    EntityId scoreRecipient = 0;
    bool found              = false;

    if (tag.hasTag(EntityTag::Player)) {
        scoreRecipient = event.attacker;
        found          = true;
    } else if (tag.hasTag(EntityTag::Projectile) && registry_->has<OwnershipComponent>(event.attacker)) {
        const auto& ownership = registry_->get<OwnershipComponent>(event.attacker);
        std::uint32_t ownerId = ownership.ownerId;

        EntityId directPlayer = static_cast<EntityId>(ownerId);
        if (registry_->isAlive(directPlayer) && registry_->has<TagComponent>(directPlayer)) {
            const auto& playerTag = registry_->get<TagComponent>(directPlayer);
            if (playerTag.hasTag(EntityTag::Player)) {
                scoreRecipient = directPlayer;
                found          = true;
            }
        }

        if (!found) {
            for (EntityId playerId : registry_->view<TagComponent, OwnershipComponent>()) {
                if (!registry_->isAlive(playerId))
                    continue;
                const auto& playerTag = registry_->get<TagComponent>(playerId);
                if (!playerTag.hasTag(EntityTag::Player))
                    continue;
                const auto& playerOwnership = registry_->get<OwnershipComponent>(playerId);
                if (playerOwnership.ownerId == ownerId) {
                    scoreRecipient = playerId;
                    found          = true;
                    break;
                }
            }
        }
    }

    if (!found) {
        return;
    }

    const auto& value = registry_->get<ScoreValueComponent>(event.target);
    if (value.value <= 0) {
        return;
    }
    if (!registry_->has<ScoreComponent>(scoreRecipient)) {
        registry_->emplace<ScoreComponent>(scoreRecipient, ScoreComponent::create(0));
    }
    registry_->get<ScoreComponent>(scoreRecipient).add(value.value);
}
