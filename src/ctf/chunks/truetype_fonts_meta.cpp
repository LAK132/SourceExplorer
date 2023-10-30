#include "truetype_fonts_meta.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t truetype_fonts_meta_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "TrueType Fonts Meta");
	}
}
