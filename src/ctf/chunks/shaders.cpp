#include "shaders.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t shaders_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Shaders");
	}
}
