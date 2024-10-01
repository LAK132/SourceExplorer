#include "extensions.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t extensions_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Extensions");
	}
}
