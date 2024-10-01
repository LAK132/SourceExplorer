#ifndef SRCEXP_CTF_CHUNKS_GLOBAL_VALUES_HPP
#define SRCEXP_CTF_CHUNKS_GLOBAL_VALUES_HPP

#include "basic.hpp"

namespace srcexp
{
	struct global_values_t : public basic_chunk_t
	{
		error_t view(instance_t &inst) const;
	};
}

#endif
