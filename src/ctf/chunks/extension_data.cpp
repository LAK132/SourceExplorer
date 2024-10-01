#include "extension_data.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t extension_data_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Extension Data");
	}
}
