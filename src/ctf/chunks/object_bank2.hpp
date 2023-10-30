#ifndef SRCEXP_CTF_CHUNKS_OBJECT_BANK2_HPP
#define SRCEXP_CTF_CHUNKS_OBJECT_BANK2_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct object_bank2_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
