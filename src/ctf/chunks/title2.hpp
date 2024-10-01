#ifndef SRCEXP_CTF_CHUNKS_TITLE2_HPP
#define SRCEXP_CTF_CHUNKS_TITLE2_HPP

#include "basic.hpp"

namespace srcexp
{
	struct title2_t : public basic_chunk_t
	{
		error_t view(instance_t &inst) const;
	};
}

#endif
