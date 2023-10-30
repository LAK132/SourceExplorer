#ifndef SRCEXP_CTF_CHUNKS_SECURITY_NUMBER_HPP
#define SRCEXP_CTF_CHUNKS_SECURITY_NUMBER_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct security_number_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
