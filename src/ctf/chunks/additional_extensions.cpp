#include "additional_extensions.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t additional_extensions_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Additional Extensions");
	}
}
