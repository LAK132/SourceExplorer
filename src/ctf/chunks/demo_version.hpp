#ifndef SRCEXP_CTF_CHUNKS_DEMO_VERSION_HPP
#define SRCEXP_CTF_CHUNKS_DEMO_VERSION_HPP

#include "basic.hpp"

namespace srcexp
{
	struct demo_version_t : public basic_chunk_t
	{
		error_t view(instance_t &inst) const;
	};
}

#endif
