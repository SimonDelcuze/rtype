#pragma once

#include <algorithm>
#include <cstdint>

struct HealthComponent
{
    std::int32_t current = 100;
    std::int32_t max     = 100;

    static HealthComponent create(std::int32_t maxHealth);
    void damage(std::int32_t amount);
    void heal(std::int32_t amount);
    bool isDead() const;
    float getPercentage() const;
};

inline HealthComponent HealthComponent::create(std::int32_t maxHealth)
{
    HealthComponent h;
    h.current = maxHealth;
    h.max     = maxHealth;
    return h;
}

inline void HealthComponent::damage(std::int32_t amount)
{
    current = std::max(0, current - amount);
}

inline void HealthComponent::heal(std::int32_t amount)
{
    current = std::min(max, current + amount);
}

inline bool HealthComponent::isDead() const
{
    return current <= 0;
}

inline float HealthComponent::getPercentage() const
{
    if (max <= 0) {
        return 0.0F;
    }
    return static_cast<float>(current) / static_cast<float>(max);
}
