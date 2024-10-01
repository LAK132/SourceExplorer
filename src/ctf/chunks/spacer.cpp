#include "spacer.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t spacer_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Spacer");
	}
}
