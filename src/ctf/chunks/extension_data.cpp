#include "extension_data.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t extension_data_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Extension Data");
	}
}
