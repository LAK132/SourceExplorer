#ifndef SRCEXP_CTF_CHUNKS_COMPRESSED_HPP
#define SRCEXP_CTF_CHUNKS_COMPRESSED_HPP

#include "basic.hpp"

namespace srcexp
{
	struct compressed_chunk_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
