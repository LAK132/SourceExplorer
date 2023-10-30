#include "string.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t string_chunk_t::read(game_t &game, data_reader_t &strm)
	{
		MEMBER_FUNCTION_CHECKPOINT();

		RES_TRY(entry.read(game, strm).RES_ADD_TRACE("string_chunk_t::read"));

		RES_TRY_TRACE_ASSIGN(value =, ReadStringEntry(game, entry));

		DEBUG(LAK_YELLOW "Value: \"", lak::to_astring(value), "\"" LAK_SGR_RESET);

		return lak::ok_t{};
	}

	error_t string_chunk_t::view(source_explorer_t &srcexp,
	                             const char *name,
	                             const bool preview) const
	{
		lak::astring str = "'" + astring() + "'";

		LAK_TREE_NODE("0x%zX %s %s##%zX",
		              (size_t)entry.ID,
		              name,
		              preview ? str.c_str() : "",
		              entry.position())
		{
			entry.view(srcexp);
			ImGui::Text("String: %s", str.c_str());
			ImGui::Text("String Length: 0x%zX", value.size());
		}

		return lak::ok_t{};
	}

	lak::u16string string_chunk_t::u16string() const { return value; }

	lak::u8string string_chunk_t::u8string() const
	{
		return lak::strconv<char8_t>(value);
	}

	lak::astring string_chunk_t::astring() const
	{
		return lak::strconv<char>(value);
	}
}
