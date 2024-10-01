#ifndef SRCEXP_CTF_CHUNKS_EXTENSION_DATA_HPP
#define SRCEXP_CTF_CHUNKS_EXTENSION_DATA_HPP

#include "basic.hpp"

namespace srcexp
{
	struct extension_data_t : public basic_chunk_t
	{
		error_t view(instance_t &inst) const;
	};
}

#endif
