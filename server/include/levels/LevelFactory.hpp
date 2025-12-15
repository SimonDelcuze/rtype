#pragma once

#include "server/ILevel.hpp"

#include <memory>

std::unique_ptr<ILevel> makeLevel(int levelId);
