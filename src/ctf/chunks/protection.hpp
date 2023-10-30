#ifndef SRCEXP_CTF_CHUNKS_PROTECTION_HPP
#define SRCEXP_CTF_CHUNKS_PROTECTION_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct protection_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
