#include "exe.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t exe_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "EXE Only");
	}
}
