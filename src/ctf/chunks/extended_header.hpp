#ifndef SRCEXP_CTF_CHUNKS_EXTENDED_HEADER_HPP
#define SRCEXP_CTF_CHUNKS_EXTENDED_HEADER_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct extended_header_t : public basic_chunk_t
	{
		uint32_t flags;
		build_type_t build_type;
		build_flags_t build_flags;
		uint16_t screen_ratio_tolerance;
		uint16_t screen_angle;

		error_t read(game_t &game, data_reader_t &strm);
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
