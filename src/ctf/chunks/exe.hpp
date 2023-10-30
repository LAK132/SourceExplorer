#ifndef SRCEXP_CTF_CHUNKS_EXE_HPP
#define SRCEXP_CTF_CHUNKS_EXE_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct exe_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
