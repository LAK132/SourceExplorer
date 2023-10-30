#include "two_five_plus_object_properties.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t two_five_plus_object_properties_t::read(game_t &game,
	                                                data_reader_t &strm)
	{
		MEMBER_FUNCTION_CHECKPOINT();

		game.two_five_plus_game = true;

		RES_TRY(entry.read(game, strm)
		          .RES_ADD_TRACE("two_five_plus_object_properties_t::read"));

		data_reader_t reader(entry.raw_body());

		while (!reader.empty())
		{
			items.emplace_back();
			RES_TRY(items.back()
			          .read(game, reader, true, 0, false)
			          .RES_ADD_TRACE("two_five_plus_object_properties_t::read"));
		}

		return lak::ok_t{};
	}

	error_t two_five_plus_object_properties_t::view(
	  source_explorer_t &srcexp) const
	{
		LAK_TREE_NODE("0x%zX Object Properties (2.5+) (%zu Items)##%zX",
		              (size_t)entry.ID,
		              items.size(),
		              entry.position())
		{
			entry.view(srcexp);

			for (const auto &item : items)
			{
				LAK_TREE_NODE("Properties##%zX", item.position())
				{
					item.view(srcexp);
				}
			}
		}

		return lak::ok_t{};
	}
}
