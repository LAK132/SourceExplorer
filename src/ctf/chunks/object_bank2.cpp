#include "object_bank2.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t object_bank2_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Object Bank 2");
	}
}
