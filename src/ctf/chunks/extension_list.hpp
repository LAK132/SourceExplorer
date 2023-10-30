#ifndef SRCEXP_CTF_CHUNKS_EXTENSION_LIST_HPP
#define SRCEXP_CTF_CHUNKS_EXTENSION_LIST_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct extension_list_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
