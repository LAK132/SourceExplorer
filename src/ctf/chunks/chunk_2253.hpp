#ifndef SRCEXP_CTF_CHUNKS_CHUNK_2253_HPP
#define SRCEXP_CTF_CHUNKS_CHUNK_2253_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct chunk_2253_item_t
	{
		size_t position;

		uint16_t ID;

		error_t read(game_t &game, data_reader_t &strm);
		error_t view(source_explorer_t &srcexp) const;
	};

	struct chunk_2253_t : public basic_chunk_t
	{
		lak::array<chunk_2253_item_t> items;

		error_t read(game_t &game, data_reader_t &strm);
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
