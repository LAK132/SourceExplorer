#include "other_extension.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t other_extension_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Other Extension");
	}
}
