#include "scheduler/ClientScheduler.hpp"

#include <stdexcept>

void ClientScheduler::addSystem(std::shared_ptr<ISystem> system)
{
    if (!system) {
        throw std::invalid_argument("Cannot add null system");
    }
    systems_.push_back(std::move(system));
    systems_.back()->initialize();
}

void ClientScheduler::update(Registry& registry, float deltaTime)
{
    for (auto& system : systems_) {
        system->update(registry, deltaTime);
    }
}

void ClientScheduler::stop()
{
    for (auto it = systems_.rbegin(); it != systems_.rend(); ++it) {
        (*it)->cleanup();
    }
    systems_.clear();
}
