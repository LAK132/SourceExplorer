#ifndef SRCEXP_CTF_CHUNKS_GLOBAL_EVENTS_HPP
#define SRCEXP_CTF_CHUNKS_GLOBAL_EVENTS_HPP

#include "basic.hpp"

namespace srcexp
{
	struct global_events_t : public basic_chunk_t
	{
		error_t view(instance_t &inst) const;
	};
}

#endif
