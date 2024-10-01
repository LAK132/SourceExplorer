#ifndef SRCEXP_CTF_CHUNKS_ICON_HPP
#define SRCEXP_CTF_CHUNKS_ICON_HPP

#include "basic.hpp"

namespace srcexp
{
	struct icon_t : public basic_chunk_t
	{
		lak::image4_t bitmap;

		error_t read(game_t &game, data_reader_t &strm);
		error_t view(instance_t &inst) const;
	};
}

#endif
