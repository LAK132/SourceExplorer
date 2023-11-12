#include "fusion_3_seed.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t fusion_3_seed_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Fusion 3 Seed");
	}
}
