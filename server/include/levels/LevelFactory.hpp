#pragma once

#include <memory>

#include "server/ILevel.hpp"

std::unique_ptr<ILevel> makeLevel(int levelId);

