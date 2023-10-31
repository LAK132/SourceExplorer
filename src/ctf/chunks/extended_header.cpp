#include "extended_header.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t extended_header_t::read(game_t &game, data_reader_t &strm)
	{
		MEMBER_FUNCTION_CHECKPOINT();

		RES_TRY(entry.read(game, strm).RES_ADD_TRACE("extended_header_t::read"));

		RES_TRY_ASSIGN(
		  auto span =,
		  entry.decode_body().RES_ADD_TRACE("extended_header_t::read"));

		data_reader_t estrm(span);

		TRY_ASSIGN(flags =, estrm.read_u32());
		TRY_ASSIGN(uint32_t _build_type =, estrm.read_u32());
		build_type = static_cast<build_type_t>(_build_type);
		TRY_ASSIGN(uint32_t _build_flags =, estrm.read_u32());
		build_flags = static_cast<build_flags_t>(_build_flags);
		TRY_ASSIGN(screen_ratio_tolerance =, estrm.read_u16());
		TRY_ASSIGN(screen_angle =, estrm.read_u16());

		game.compat |= (size_t)build_type >= 0x10000000;

		return lak::ok_t{};
	}

	error_t extended_header_t::view(source_explorer_t &srcexp) const
	{
		LAK_TREE_NODE(
		  "0x%zX Extended Header##%zX", (size_t)entry.ID, entry.position())
		{
			entry.view(srcexp);

			ImGui::Text("Flags: 0x%zX", (size_t)flags);
			ImGui::Text("Build Type: 0x%zX", (size_t)build_type);
			ImGui::Text("Build Flags: %s (0x%zX)",
			            GetBuildFlagsString(build_flags).c_str(),
			            (size_t)build_flags);
			ImGui::Text("Screen Ratio Tolerance: 0x%zX",
			            (size_t)screen_ratio_tolerance);
			ImGui::Text("Screen Angle: 0x%zX", (size_t)screen_angle);
		}

		return lak::ok_t{};
	}
}
