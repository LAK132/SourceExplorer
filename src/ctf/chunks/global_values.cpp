#include "global_values.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t global_values_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Global Values");
	}
}
