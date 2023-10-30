#include "extension_data.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t extension_data_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Extension Data");
	}
}
