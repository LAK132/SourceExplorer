#ifndef SRCEXP_CTF_CHUNKS_SPACER_HPP
#define SRCEXP_CTF_CHUNKS_SPACER_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct spacer_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
