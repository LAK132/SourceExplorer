#include "extension_path.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t extension_path_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Extension Path");
	}
}
