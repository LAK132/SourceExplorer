#ifndef SRCEXP_CTF_CHUNKS_VITALISE_PREVIEW_HPP
#define SRCEXP_CTF_CHUNKS_VITALISE_PREVIEW_HPP

#include "basic.hpp"

namespace srcexp
{
	struct vitalise_preview_t : public basic_chunk_t
	{
		error_t view(instance_t &inst) const;
	};
}

#endif
