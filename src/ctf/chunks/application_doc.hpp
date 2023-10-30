#ifndef SRCEXP_CTF_CHUNKS_APPLICATION_DOC_HPP
#define SRCEXP_CTF_CHUNKS_APPLICATION_DOC_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct application_doc_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
