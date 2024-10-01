#ifndef SRCEXP_CTF_CHUNKS_MENU_HPP
#define SRCEXP_CTF_CHUNKS_MENU_HPP

#include "basic.hpp"

namespace srcexp
{
	struct menu_t : public basic_chunk_t
	{
		error_t view(instance_t &inst) const;
	};
}

#endif
