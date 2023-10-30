#ifndef SRCEXP_CTF_CHUNKS_BINARY_FILES_HPP
#define SRCEXP_CTF_CHUNKS_BINARY_FILES_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct binary_files_item_t
	{
		lak::u8string name;
		data_ref_span_t data;

		error_t read(game_t &game, data_reader_t &strm);
		error_t view(source_explorer_t &srcexp) const;
	};

	struct binary_files_t : public basic_chunk_t
	{
		lak::array<binary_files_item_t> items;

		error_t read(game_t &game, data_reader_t &strm);
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
