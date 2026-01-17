#pragma once

#include <cstdint>

	struct ShieldComponent
	{
	    std::uint32_t ownerId = 0;
	    int hitsRemaining     = 5;

	    static constexpr int kShieldCost = 5000;
	    static constexpr int kMaxHits    = 5;

	    static ShieldComponent create(std::uint32_t owner);
	};
