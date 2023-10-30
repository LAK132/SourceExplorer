#ifndef SRCEXP_CTF_CHUNKS_TRUETYPE_FONTS_META_HPP
#define SRCEXP_CTF_CHUNKS_TRUETYPE_FONTS_META_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct truetype_fonts_meta_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
