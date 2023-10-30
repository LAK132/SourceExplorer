#ifndef SRCEXP_CTF_CHUNKS_VITALISE_PREVIEW_HPP
#define SRCEXP_CTF_CHUNKS_VITALISE_PREVIEW_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct vitalise_preview_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
