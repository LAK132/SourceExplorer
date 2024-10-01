#include "vitalise_preview.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t vitalise_preview_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Vitalise Preview");
	}
}
