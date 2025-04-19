#include "extended_header.hpp"

#include "../explorer.hpp"

namespace srcexp
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

		game.compat |= (size_t)build_type >= 0x1000'0000;
		switch (build_type)
		{
			case build_type_t::windows_exe:
			case build_type_t::windows_screen_saver:
			case build_type_t::sub_application:
			case build_type_t::uwp_project:
				DEBUG("Windows Host");
			default:
				game.host = host_system_t::windows;
				break;

			case build_type_t::java_sub_application:
			case build_type_t::java_application:
			case build_type_t::java_internet_applet:
			case build_type_t::java_web_start:
			case build_type_t::java_for_mobile_devices:
			case build_type_t::java_mac_application:
			case build_type_t::java_for_blackberry:
				DEBUG("Java Host");
				game.host = host_system_t::java;
				break;

			case build_type_t::adobe_flash:
				DEBUG("Flash Host");
				game.host = host_system_t::flash;
				break;

			case build_type_t::xna_windows_project:
			case build_type_t::xna_xbox_project:
			case build_type_t::xna_phone_project:
				DEBUG("XNA Host");
				game.host = host_system_t::xna;
				break;

			case build_type_t::html5_devel:
			case build_type_t::html5_final:
				DEBUG("HTML5 Host");
				game.host = host_system_t::html5;
				break;

			case build_type_t::android_ouya_application:
			case build_type_t::android_app_bundle:
				DEBUG("Android Host");
				game.host = host_system_t::android;
				break;

			case build_type_t::ios_application:
			case build_type_t::ios_xcode_devel:
			case build_type_t::ios_xcode_final:
				DEBUG("iOS Host");
				game.host = host_system_t::ios;
				break;

			case build_type_t::nintendo_switch:
				DEBUG("Switch Host");
				game.host = host_system_t::nintendo_switch;
				break;

			case build_type_t::xbox_one:
				DEBUG("Xbone Host");
				game.host = host_system_t::xbox_one;
				break;

			case build_type_t::playstation:
				DEBUG("PS Host");
				game.host = host_system_t::playstation;
				break;
		}

		return lak::ok_t{};
	}

	error_t extended_header_t::view(instance_t &inst) const
	{
		LAK_TREE_NODE(
		  "0x%zX Extended Header##%zX", (size_t)entry.ID, entry.position())
		{
			entry.view(inst);

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
