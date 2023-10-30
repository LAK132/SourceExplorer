#include "chunk_2255.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t chunk_2255_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Chunk 2255");
	}
}
