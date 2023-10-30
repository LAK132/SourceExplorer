#include "global_strings.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t global_strings_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Global Strings");
	}
}
