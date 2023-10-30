#ifndef SRCEXP_CTF_CHUNKS_STRING_HPP
#define SRCEXP_CTF_CHUNKS_STRING_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct string_chunk_t : public basic_chunk_t
	{
		mutable std::u16string value;

		error_t read(game_t &game, data_reader_t &strm);
		error_t view(source_explorer_t &srcexp,
		             const char *name,
		             const bool preview = false) const;

		lak::u16string u16string() const;
		lak::u8string u8string() const;
		lak::astring astring() const;
	};
}

#endif
