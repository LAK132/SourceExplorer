#include "truetype_fonts_meta.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t truetype_fonts_meta_t::view(instance_t &inst) const
	{
		return basic_view(inst, "TrueType Fonts Meta");
	}
}
