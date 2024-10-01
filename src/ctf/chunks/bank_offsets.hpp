#ifndef SRCEXP_CTF_CHUNKS_BANK_OFFSETS_HPP
#define SRCEXP_CTF_CHUNKS_BANK_OFFSETS_HPP

#include "basic.hpp"

namespace srcexp
{
	struct bank_offsets_t : public basic_chunk_t
	{
		error_t view(instance_t &inst) const;
	};
}

#endif
