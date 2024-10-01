#include "bank_offsets.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t bank_offsets_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Bank Offsets");
	}
}
