#include "menu.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t menu_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Menu");
	}
}
