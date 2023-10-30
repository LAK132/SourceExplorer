#include "demo_version.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t demo_version_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Demo Version");
	}
}
