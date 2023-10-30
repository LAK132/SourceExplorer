#include "spacer.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t spacer_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Spacer");
	}
}
