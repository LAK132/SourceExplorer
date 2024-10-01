#ifndef SRCEXP_CTF_CHUNKS_ADDITIONAL_EXTENSIONS_HPP
#define SRCEXP_CTF_CHUNKS_ADDITIONAL_EXTENSIONS_HPP

#include "basic.hpp"

namespace srcexp
{
	struct additional_extensions_t : public basic_chunk_t
	{
		error_t view(instance_t &inst) const;
	};
}

#endif
