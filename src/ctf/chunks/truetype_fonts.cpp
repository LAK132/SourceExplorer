#include "truetype_fonts.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t truetype_fonts_t::read(game_t &game, data_reader_t &strm)
	{
		MEMBER_FUNCTION_CHECKPOINT();

		RES_TRY(entry.read(game, strm).RES_ADD_TRACE("truetype_fonts_t::read"));

		data_reader_t reader(entry.raw_body());

		for (size_t i = 0; !reader.empty(); ++i)
		{
			DEBUG("Font ", i);
			items.emplace_back();
			RES_TRY(items.back()
			          .read(game, reader, false)
			          .RES_ADD_TRACE("truetype_fonts_t::read"));
		}

		return lak::ok_t{};
	}

	error_t truetype_fonts_t::view(instance_t &inst) const
	{
		LAK_TREE_NODE("0x%zX TrueType Fonts (%zu Items)##%zX",
		              (size_t)entry.ID,
		              items.size(),
		              entry.position())
		{
			entry.view(inst);

			for (const auto &item : items)
			{
				LAK_TREE_NODE("0x%zX Font##%zX", (size_t)item.ID, item.position())
				{
					item.view(inst);
				}
			}
		}

		return lak::ok_t{};
	}
}
