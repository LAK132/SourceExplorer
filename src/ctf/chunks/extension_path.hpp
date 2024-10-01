#ifndef SRCEXP_CTF_CHUNKS_EXTENSION_PATH_HPP
#define SRCEXP_CTF_CHUNKS_EXTENSION_PATH_HPP

#include "basic.hpp"

namespace srcexp
{
	struct extension_path_t : public basic_chunk_t
	{
		error_t view(instance_t &inst) const;
	};
}

#endif
