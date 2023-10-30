#include "security_number.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t security_number_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Security Number");
	}
}
