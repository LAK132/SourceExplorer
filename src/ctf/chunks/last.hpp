#ifndef SRCEXP_CTF_CHUNKS_LAST_HPP
#define SRCEXP_CTF_CHUNKS_LAST_HPP

#include "basic.hpp"

namespace srcexp
{
	struct last_t : public basic_chunk_t
	{
		error_t view(instance_t &inst) const;
	};
}

#endif
