#pragma once

#include <cstdint>

struct EnemyShootingComponent
{
    float shootInterval      = 1.5F;
    float timeSinceLastShot  = 0.0F;
    float projectileSpeed    = 300.0F;
    std::int32_t projectileDamage = 5;
    float projectileLifetime = 3.0F;

    static EnemyShootingComponent create(float interval = 1.5F, float speed = 300.0F, 
                                         std::int32_t damage = 5, float lifetime = 3.0F);
};

inline EnemyShootingComponent EnemyShootingComponent::create(float interval, float speed, 
                                                              std::int32_t damage, float lifetime)
{
    EnemyShootingComponent component;
    component.shootInterval = interval;
    component.timeSinceLastShot = 0.0F;
    component.projectileSpeed = speed;
    component.projectileDamage = damage;
    component.projectileLifetime = lifetime;
    return component;
}
