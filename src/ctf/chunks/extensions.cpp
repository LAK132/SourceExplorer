#include "extensions.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t extensions_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Extensions");
	}
}
