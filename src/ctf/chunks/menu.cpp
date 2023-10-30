#include "menu.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t menu_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Menu");
	}
}
