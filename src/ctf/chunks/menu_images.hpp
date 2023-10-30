#ifndef SRCEXP_CTF_CHUNKS_MENU_IMAGES_HPP
#define SRCEXP_CTF_CHUNKS_MENU_IMAGES_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	struct menu_images_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};
}

#endif
