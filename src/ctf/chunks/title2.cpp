#include "title2.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t title2_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Title 2");
	}
}
