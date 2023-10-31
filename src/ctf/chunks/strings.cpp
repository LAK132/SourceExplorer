#include "strings.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t strings_chunk_t::read(game_t &game, data_reader_t &strm)
	{
		MEMBER_FUNCTION_CHECKPOINT();

		RES_TRY(entry.read(game, strm).RES_ADD_TRACE("strings_chunk_t::read"));

		RES_TRY_ASSIGN(auto span =,
		               entry.decode_body().RES_ADD_TRACE("strings_chunk_t::read"));

		data_reader_t sstrm(span);

		while (!sstrm.empty())
		{
			auto str = sstrm.read_any_c_str<char16_t>();
			DEBUG("    Read String (Len ", str.size(), ")");
			if (!str.size()) break;
			values.emplace_back(lak::move(str));
		}

		return lak::ok_t{};
	}

	error_t strings_chunk_t::basic_view(source_explorer_t &srcexp,
	                                    const char *name) const
	{
		LAK_TREE_NODE("0x%zX %s (%zu Items)##%zX",
		              (size_t)entry.ID,
		              name,
		              values.size(),
		              entry.position())
		{
			entry.view(srcexp);
			for (const auto &s : values)
				ImGui::Text("%s", (const char *)lak::to_u8string(s).c_str());
		}

		return lak::ok_t{};
	}

	error_t strings_chunk_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Unknown Strings");
	}
}
