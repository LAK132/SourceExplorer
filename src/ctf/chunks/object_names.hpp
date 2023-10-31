#ifndef SRCEXP_CTF_CHUNKS_OBJECT_NAMES_HPP
#define SRCEXP_CTF_CHUNKS_OBJECT_NAMES_HPP

#include "basic.hpp"

#include "strings.hpp"

namespace SourceExplorer
{
	struct object_names_t : public strings_chunk_t
	{
		error_t read(game_t &game, data_reader_t &strm);
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
