#include "application_doc.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t application_doc_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Appliocation Doc");
	}
}
