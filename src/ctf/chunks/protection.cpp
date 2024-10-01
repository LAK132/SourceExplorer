#include "protection.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t protection_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Protection");
	}
}
