#pragma once

#include <cstdint>

	struct AllyComponent
	{
	    std::uint32_t ownerId = 0;
	    float shootTimer      = 0.0F;

	    static constexpr float kShootInterval = 2.0F;
	    static constexpr int kAllyCost        = 10000;

	    static AllyComponent create(std::uint32_t owner);
	};
