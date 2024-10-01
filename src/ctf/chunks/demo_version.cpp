#include "demo_version.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t demo_version_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Demo Version");
	}
}
