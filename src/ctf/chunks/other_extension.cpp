#include "other_extension.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t other_extension_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Other Extension");
	}
}
