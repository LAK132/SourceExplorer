#ifndef SRCEXP_CTF_CHUNKS_TITLE2_HPP
#define SRCEXP_CTF_CHUNKS_TITLE2_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct title2_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
