#include "global_events.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t global_events_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Global Events");
	}
}
