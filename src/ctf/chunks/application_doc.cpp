#include "application_doc.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t application_doc_t::view(instance_t &inst) const
	{
		return basic_view(inst, "Appliocation Doc");
	}
}
