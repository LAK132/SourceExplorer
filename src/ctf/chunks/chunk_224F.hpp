#ifndef SRCEXP_CTF_CHUNKS_CHUNK_224F_HPP
#define SRCEXP_CTF_CHUNKS_CHUNK_224F_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct chunk_224F_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
