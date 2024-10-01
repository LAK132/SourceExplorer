#ifndef SRCEXP_CTF_CHUNKS_SECURITY_NUMBER_HPP
#define SRCEXP_CTF_CHUNKS_SECURITY_NUMBER_HPP

#include "basic.hpp"

namespace srcexp
{
	struct security_number_t : public basic_chunk_t
	{
		error_t view(instance_t &inst) const;
	};
}

#endif
