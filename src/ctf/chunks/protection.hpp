#ifndef SRCEXP_CTF_CHUNKS_PROTECTION_HPP
#define SRCEXP_CTF_CHUNKS_PROTECTION_HPP

#include "basic.hpp"

namespace srcexp
{
	struct protection_t : public basic_chunk_t
	{
		error_t view(instance_t &inst) const;
	};
}

#endif
