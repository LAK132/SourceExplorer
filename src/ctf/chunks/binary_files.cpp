#include "binary_files.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t binary_files_item_t::read(game_t &game, data_reader_t &strm)
	{
		MEMBER_FUNCTION_CHECKPOINT();

		TRY_ASSIGN(const auto str_len =, strm.read_u16());

		auto to_str = [](auto &&arr)
		{ return lak::to_u8string(lak::string_view(lak::span(arr))); };
		auto set_name = [&](auto &&str) { name = lak::move(str); };
		if (game.unicode)
		{
			TRY(strm.read<char16_t>(str_len).map(to_str).if_ok(set_name));
		}
		else
		{
			TRY(strm.read<char>(str_len).map(to_str).if_ok(set_name));
		}

		TRY_ASSIGN(const auto data_len =, strm.read_u32());

		TRY_ASSIGN(data =, strm.read_ref_span(data_len));

		return lak::ok_t{};
	}

	error_t binary_files_item_t::view(source_explorer_t &) const
	{
		auto str = lak::as_astring(name);

		LAK_TREE_NODE("%s", str.data())
		{
			ImGui::Text("Name: %s", str.data());
			ImGui::Text("Data Size: 0x%zX", data.size());
		}

		return lak::ok_t{};
	}

	error_t binary_files_t::read(game_t &game, data_reader_t &strm)
	{
		MEMBER_FUNCTION_CHECKPOINT();

		RES_TRY(entry.read(game, strm).RES_ADD_TRACE("binary_files_t::read"));

		RES_TRY_ASSIGN(auto span =,
		               entry.decode_body().RES_ADD_TRACE("binary_files_t::read"));

		data_reader_t bstrm(span);

		TRY_ASSIGN(const auto item_count =, bstrm.read_u32());

		items.resize(item_count);
		for (auto &item : items)
		{
			RES_TRY(item.read(game, bstrm).RES_ADD_TRACE("binary_files_t::read"));
		}

		return lak::ok_t{};
	}

	error_t binary_files_t::view(source_explorer_t &srcexp) const
	{
		LAK_TREE_NODE(
		  "0x%zX Binary Files##%zX", (size_t)entry.ID, entry.position())
		{
			entry.view(srcexp);

			int index = 0;
			for (const auto &item : items)
			{
				ImGui::PushID(index++);
				DEFER(ImGui::PopID());
				RES_TRY(item.view(srcexp).RES_ADD_TRACE("binary_files_t::view"));
			}
		}

		return lak::ok_t{};
	}
}
