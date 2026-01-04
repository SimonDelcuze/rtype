#include "systems/IntroCinematicSystem.hpp"

#include "level/IntroCinematicConfig.hpp"

IntroCinematicSystem::IntroCinematicSystem(LevelState& state) : state_(&state) {}

void IntroCinematicSystem::initialize() {}

void IntroCinematicSystem::update(Registry&, float deltaTime)
{
    if (state_ == nullptr || !state_->introCinematicActive) {
        return;
    }
    state_->introCinematicTime += deltaTime;
    const float total = kIntroCinematicConfig.totalDuration();
    if (state_->introCinematicTime >= total) {
        state_->introCinematicTime   = total;
        state_->introCinematicActive = false;
    }
}

void IntroCinematicSystem::cleanup() {}
