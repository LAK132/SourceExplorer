#include "exe.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t exe_t::view(instance_t &inst) const
	{
		return basic_view(inst, "EXE Only");
	}
}
