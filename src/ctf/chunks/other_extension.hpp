#ifndef SRCEXP_CTF_CHUNKS_OTHER_EXTENSIONS_HPP
#define SRCEXP_CTF_CHUNKS_OTHER_EXTENSIONS_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct other_extension_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
