#include "vitalise_preview.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t vitalise_preview_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Vitalise Preview");
	}
}
