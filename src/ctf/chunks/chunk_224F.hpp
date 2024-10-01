#ifndef SRCEXP_CTF_CHUNKS_CHUNK_224F_HPP
#define SRCEXP_CTF_CHUNKS_CHUNK_224F_HPP

#include "basic.hpp"

namespace srcexp
{
	struct chunk_224F_t : public basic_chunk_t
	{
		error_t view(instance_t &inst) const;
	};
}

#endif
