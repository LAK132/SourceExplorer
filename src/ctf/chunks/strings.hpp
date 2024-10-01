#ifndef SRCEXP_CTF_CHUNKS_STRINGS_HPP
#define SRCEXP_CTF_CHUNKS_STRINGS_HPP

#include "basic.hpp"

namespace srcexp
{
	struct strings_chunk_t : public basic_chunk_t
	{
		mutable std::vector<std::u16string> values;

		error_t read(game_t &game, data_reader_t &strm);
		error_t basic_view(instance_t &inst, const char *name) const;
		error_t view(instance_t &inst) const;
	};
}

#endif
