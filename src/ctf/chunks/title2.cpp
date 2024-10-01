#include "title2.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t title2_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Title 2");
	}
}
