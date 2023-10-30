#include "object_names.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t object_names_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Object Names");
	}
}
