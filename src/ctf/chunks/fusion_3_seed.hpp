#ifndef SRCEXP_CTF_CHUNKS_FUSION_3_SEED_HPP
#define SRCEXP_CTF_CHUNKS_FUSION_3_SEED_HPP

#include "basic.hpp"

namespace srcexp
{
	struct fusion_3_seed_t : public basic_chunk_t
	{
		error_t view(instance_t &inst) const;
	};
}

#endif
