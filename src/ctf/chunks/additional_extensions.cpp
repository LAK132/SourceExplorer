#include "additional_extensions.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t additional_extensions_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Additional Extensions");
	}
}
