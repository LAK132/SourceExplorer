#include "extension_list.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t extension_list_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Extension List");
	}
}
