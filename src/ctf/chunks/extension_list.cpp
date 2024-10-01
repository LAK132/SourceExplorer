#include "extension_list.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t extension_list_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Extension List");
	}
}
