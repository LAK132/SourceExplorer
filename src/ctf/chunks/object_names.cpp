#include "object_names.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t object_names_t::read(game_t &game, data_reader_t &strm)
	{
		MEMBER_FUNCTION_CHECKPOINT();

		RES_TRY(entry.read(game, strm).RES_ADD_TRACE("object_names_t::read"));

		RES_TRY_ASSIGN(auto span =,
		               entry.decode_body().RES_ADD_TRACE("object_names_t::read"));

		data_reader_t sstrm(span);

		while (!sstrm.empty())
		{
			auto str = game.cruf ? lak::to_u16string(sstrm.read_any_c_str<char8_t>())
			                     : sstrm.read_any_c_str<char16_t>();
			DEBUG("    Read String (Len ", str.size(), ")");
			if (!str.size()) break;
			values.emplace_back(lak::move(str));
		}

		return lak::ok_t{};
	}

	error_t object_names_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Object Names");
	}
}
