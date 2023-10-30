#ifndef SRCEXP_CTF_CHUNKS_DEMO_VERSION_HPP
#define SRCEXP_CTF_CHUNKS_DEMO_VERSION_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct demo_version_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
