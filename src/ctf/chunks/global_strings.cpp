#include "global_strings.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t global_strings_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Global Strings");
	}
}
