#include "security_number.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t security_number_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Security Number");
	}
}
