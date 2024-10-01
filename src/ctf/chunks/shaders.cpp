#include "shaders.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t shaders_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Shaders");
	}
}
