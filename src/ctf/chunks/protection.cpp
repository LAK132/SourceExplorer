#include "protection.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t protection_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Protection");
	}
}
