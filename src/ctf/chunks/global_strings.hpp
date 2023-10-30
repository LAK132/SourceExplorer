#ifndef SRCEXP_CTF_CHUNKS_GLOBAL_STRINGS_HPP
#define SRCEXP_CTF_CHUNKS_GLOBAL_STRINGS_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct global_strings_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
