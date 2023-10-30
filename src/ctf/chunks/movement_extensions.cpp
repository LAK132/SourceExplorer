#include "movement_extensions.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t movement_extensions_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Movement Extensions");
	}
}
