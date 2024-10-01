#ifndef SRCEXP_CTF_CHUNKS_SHADERS_HPP
#define SRCEXP_CTF_CHUNKS_SHADERS_HPP

#include "basic.hpp"

namespace srcexp
{
	struct shaders_t : public basic_chunk_t
	{
		error_t view(instance_t &inst) const;
	};
}

#endif
