#ifndef SRCEXP_CTF_CHUNKS_FUSION_3_SEED_HPP
#define SRCEXP_CTF_CHUNKS_FUSION_3_SEED_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct fusion_3_seed_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
