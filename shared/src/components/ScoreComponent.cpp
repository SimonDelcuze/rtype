#include "components/ScoreComponent.hpp"

ScoreComponent ScoreComponent::create(int initial)
{
    ScoreComponent s;
    s.value = initial;
    return s;
}
