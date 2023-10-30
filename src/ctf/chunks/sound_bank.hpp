#ifndef SRCEXP_CTF_CHUNKS_SOUND_BANK_HPP
#define SRCEXP_CTF_CHUNKS_SOUND_BANK_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	namespace sound
	{
		struct item_t : public basic_item_t
		{
			uint32_t checksum;
			uint32_t references;
			uint32_t decomp_len;
			uint32_t type;
			uint32_t reserved;
			uint32_t name_len;

			error_t read(game_t &game, data_reader_t &strm);
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
