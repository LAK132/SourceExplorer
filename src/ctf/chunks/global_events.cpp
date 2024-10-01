#include "global_events.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t global_events_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Global Events");
	}
}
