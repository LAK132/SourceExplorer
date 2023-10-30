#ifndef SRCEXP_CTF_CHUNKS_GLOBAL_VALUES_HPP
#define SRCEXP_CTF_CHUNKS_GLOBAL_VALUES_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct global_values_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
