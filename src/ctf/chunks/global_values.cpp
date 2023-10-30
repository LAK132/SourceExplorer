#include "global_values.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t global_values_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Global Values");
	}
}
