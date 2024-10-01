#ifndef SRCEXP_CTF_CHUNKS_CHUNK_2255_HPP
#define SRCEXP_CTF_CHUNKS_CHUNK_2255_HPP

#include "basic.hpp"

namespace srcexp
{
	struct chunk_2255_t : public basic_chunk_t
	{
		error_t view(instance_t &inst) const;
	};
}

#endif
