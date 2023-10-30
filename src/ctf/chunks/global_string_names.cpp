#include "global_string_names.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t global_string_names_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Global String Names");
	}
}
