#ifndef SRCEXP_CTF_CHUNKS_GLOBAL_STRING_NAMES_HPP
#define SRCEXP_CTF_CHUNKS_GLOBAL_STRING_NAMES_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct global_string_names_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
