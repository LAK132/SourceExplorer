#include "last.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t last_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Last");
	}
}
