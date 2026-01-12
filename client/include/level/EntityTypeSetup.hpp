#pragma once

#include "assets/AssetManifest.hpp"
#include "graphics/TextureManager.hpp"
#include "level/EntityTypeRegistry.hpp"

class AnimationRegistry;
struct AnimationLabels;

void registerEntityTypes(EntityTypeRegistry& registry, TextureManager& textures, const AssetManifest& manifest,
                         const AnimationRegistry* animations = nullptr, const AnimationLabels* labels = nullptr);
