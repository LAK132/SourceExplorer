#include "menu_images.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t menu_images_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Menu Images");
	}
}
