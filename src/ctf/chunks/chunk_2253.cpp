#include "chunk_2253.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t chunk_2253_item_t::read(game_t &, data_reader_t &strm)
	{
		MEMBER_FUNCTION_CHECKPOINT();

		CHECK_REMAINING(strm, 16U);

		position = strm.position();
		DEFER(strm.seek(position + 16).UNWRAP());

		TRY_ASSIGN(ID =, strm.read_u16());

		return lak::ok_t{};
	}

	error_t chunk_2253_item_t::view(source_explorer_t &) const
	{
		ImGui::Text("ID: 0x%zX", size_t(ID));

		return lak::ok_t{};
	}

	error_t chunk_2253_t::read(game_t &game, data_reader_t &strm)
	{
		MEMBER_FUNCTION_CHECKPOINT();

		RES_TRY(entry.read(game, strm).RES_ADD_TRACE("chunk_2253_t::read"));

		RES_TRY_ASSIGN(auto span =,
		               entry.decode_body().RES_ADD_TRACE("chunk_2253_t::read"));

		data_reader_t cstrm(span);

		while (cstrm.remaining().size() >= 16)
		{
			const size_t pos = cstrm.position();
			items.emplace_back();
			RES_TRY(
			  items.back().read(game, cstrm).RES_ADD_TRACE("chunk_2253_t::read"));
			if (cstrm.position() == pos) break;
		}

		if (!cstrm.empty()) WARNING("Data Left Over");

		return lak::ok_t{};
	}

	error_t chunk_2253_t::view(source_explorer_t &srcexp) const
	{
		LAK_TREE_NODE("0x%zX Chunk 2253 (%zu Items)##%zX",
		              (size_t)entry.ID,
		              items.size(),
		              entry.position())
		{
			entry.view(srcexp);

			size_t i = 0;
			for (const auto &item : items)
			{
				LAK_TREE_NODE("0x%zX Item##%zX", (size_t)item.ID, i++)
				{
					RES_TRY(item.view(srcexp).RES_ADD_TRACE("chunk_2253_t::view"));
				}
			}
		}

		return lak::ok_t{};
	}
}
