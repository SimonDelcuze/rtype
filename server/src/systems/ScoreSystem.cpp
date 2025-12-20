#include "systems/ScoreSystem.hpp"

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
    if (!tag.hasTag(EntityTag::Player)) {
        return;
    }
    const auto& value = registry_->get<ScoreValueComponent>(event.target);
    if (value.value <= 0) {
        return;
    }
    if (!registry_->has<ScoreComponent>(event.attacker)) {
        registry_->emplace<ScoreComponent>(event.attacker, ScoreComponent::create(0));
    }
    registry_->get<ScoreComponent>(event.attacker).add(value.value);
}
