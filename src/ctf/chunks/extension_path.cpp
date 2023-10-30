#include "extension_path.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t extension_path_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Extension Path");
	}
}
