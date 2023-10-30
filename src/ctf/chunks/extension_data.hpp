#ifndef SRCEXP_CTF_CHUNKS_EXTENSION_DATA_HPP
#define SRCEXP_CTF_CHUNKS_EXTENSION_DATA_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct extension_data_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
