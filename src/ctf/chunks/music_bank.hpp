#ifndef SRCEXP_CTF_CHUNKS_MUSIC_BANK_HPP
#define SRCEXP_CTF_CHUNKS_MUSIC_BANK_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	namespace music
	{
		struct item_t : public basic_item_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct end_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct bank_t : public basic_chunk_t
		{
			lak::array<item_t> items;
			lak::unique_ptr<end_t> end;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};
	}
}

#endif
