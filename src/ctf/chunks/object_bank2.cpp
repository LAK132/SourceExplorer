#include "object_bank2.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t object_bank2_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Object Bank 2");
	}
}
