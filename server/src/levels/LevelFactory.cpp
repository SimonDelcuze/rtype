#include "levels/LevelFactory.hpp"

#include "levels/Level1.hpp"

std::unique_ptr<ILevel> makeLevel(int levelId)
{
    switch (levelId) {
        case 1:
        default:
            return std::make_unique<Level1>();
    }
}

