#pragma once

#include "scheduler/IScheduler.hpp"

#include <memory>
#include <vector>

class ClientScheduler : public IScheduler
{
  public:
    void addSystem(std::shared_ptr<ISystem> system) override;
    void update(Registry& registry, float deltaTime) override;
    void stop() override;

  private:
    std::vector<std::shared_ptr<ISystem>> systems_;
};
