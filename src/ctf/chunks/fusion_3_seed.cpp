#include "fusion_3_seed.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t fusion_3_seed_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Fusion 3 Seed");
	}
}
