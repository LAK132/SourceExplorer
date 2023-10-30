#ifndef SRCEXP_CTF_CHUNKS_TRUETYPE_FONTS_HPP
#define SRCEXP_CTF_CHUNKS_TRUETYPE_FONTS_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct truetype_fonts_t : public basic_chunk_t
	{
		lak::array<item_entry_t> items;

		error_t read(game_t &game, data_reader_t &strm);
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
