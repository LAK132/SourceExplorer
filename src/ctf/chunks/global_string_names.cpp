#include "global_string_names.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t global_string_names_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Global String Names");
	}
}
