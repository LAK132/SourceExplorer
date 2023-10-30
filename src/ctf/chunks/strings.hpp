#ifndef SRCEXP_CTF_CHUNKS_STRINGS_HPP
#define SRCEXP_CTF_CHUNKS_STRINGS_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct strings_chunk_t : public basic_chunk_t
	{
		mutable std::vector<std::u16string> values;

		error_t read(game_t &game, data_reader_t &strm);
		error_t basic_view(source_explorer_t &srcexp, const char *name) const;
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
