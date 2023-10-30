#include "last.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t last_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Last");
	}
}
