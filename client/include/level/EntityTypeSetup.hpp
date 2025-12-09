#pragma once

#include "assets/AssetManifest.hpp"
#include "graphics/TextureManager.hpp"
#include "level/EntityTypeRegistry.hpp"

void registerEntityTypes(EntityTypeRegistry& registry, TextureManager& textures, const AssetManifest& manifest);
