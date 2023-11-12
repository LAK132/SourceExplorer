#include "bank_offsets.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t bank_offsets_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Bank Offsets");
	}
}
