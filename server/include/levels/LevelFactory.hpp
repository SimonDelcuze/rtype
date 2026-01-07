#pragma once

#include "levels/ILevel.hpp"

#include <memory>

std::unique_ptr<ILevel> makeLevel(int levelId);
