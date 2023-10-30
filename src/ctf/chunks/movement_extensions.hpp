#ifndef SRCEXP_CTF_CHUNKS_MOVEMENT_EXTENSIONS_HPP
#define SRCEXP_CTF_CHUNKS_MOVEMENT_EXTENSIONS_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct movement_extensions_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
