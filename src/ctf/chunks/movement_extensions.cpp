#include "movement_extensions.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t movement_extensions_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Movement Extensions");
	}
}
