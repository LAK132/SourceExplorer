// Copyright (c) Mathias Kaerlev 2012, LAK132 2019

// This file is part of Anaconda.

// Anaconda is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Anaconda is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Anaconda.  If not, see <http://www.gnu.org/licenses/>.

// This is here to stop the #define ERROR clash caused by wingdi
#include <GL/gl3w.h>
#include <SDL2/SDL.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_utils.hpp"

#include "dump.h"
#include "main.h"

#include "byte_pairs_window.hpp"
#include "main_window.hpp"
#include "testing_window.hpp"

#include <lak/opengl/shader.hpp>
#include <lak/opengl/state.hpp>
#include <lak/opengl/texture.hpp>

#include <lak/bank_ptr.hpp>
#include <lak/defer.hpp>
#include <lak/file.hpp>
#include <lak/string_literals.hpp>
#include <lak/string_utils.hpp>
#include <lak/test.hpp>
#include <lak/window.hpp>

#ifndef MAXDIRLEN
#	define MAXDIRLEN 512
#endif

se::source_explorer_t SrcExp;
int opengl_major, opengl_minor;
se_main_mode_t se_main_mode = se_main_mode_t::normal;

#if 1
void MainScreen(float frame_time)
{
	switch (se_main_mode)
	{
		case se_main_mode_t::byte_pairs:
			byte_pairs_window::draw(frame_time);
			break;

		case se_main_mode_t::testing: test_window::draw(frame_time); break;

		case se_main_mode_t::normal: [[fallthrough]];
		default: main_window::draw(frame_time); break;
	}
}

#else
void FloatThing(lak::memory &block)
{
	if (auto *ptr = block.read_type<float>(); ptr)
		ImGui::DragFloat("FloatThing", ptr);
}

std::vector<void (*)(lak::memory &block)> funcs = {&FloatThing};

void MainScreen(float frame_time)
{
	if (ImGui::BeginMenuBar())
	{
		ImGui::EndMenuBar();
	}

	float f = 0.0;

	lak::memory block;
	block.write_type(&f);
	block.position = 0;

	for (auto *func : funcs) func(block);
}
#endif

#include <lak/basic_program.inl>

ImGui::ImplContext imgui_context = nullptr;

bool force_only_error = false;

lak::optional<int> basic_window_preinit(int argc, char **argv)
{
	if (argc == 2 && argv[1] == lak::astring("--version"))
	{
		std::cout << "Source Explorer " APP_VERSION << "\n";
		return lak::optional<int>(0);
	}
	else if (argc == 2 && argv[1] == lak::astring("--full-version"))
	{
		std::cout << APP_NAME << "\n";
		return lak::optional<int>(0);
	}

	lak::debugger.std_out(u8"", u8"" APP_NAME "\n");

	for (int arg = 1; arg < argc; ++arg)
	{
		if (argv[arg] == lak::astring("-help"))
		{
			std::cout
			  << "srcexp.exe [-help] [-nogl] [-onlyerr] "
			     "[-listtests | -testall | -tests \"test1;test2\"] [<filepath>]\n";
			return lak::optional<int>(0);
		}
		else if (argv[arg] == lak::astring("-nogl"))
		{
			basic_window_force_software = true;
		}
		else if (argv[arg] == lak::astring("-onlyerr"))
		{
			force_only_error = true;
		}
		else if (argv[arg] == lak::astring("-listtests"))
		{
			lak::debugger.std_out(lak::u8string(),
			                      lak::u8string(u8"Available tests:\n"));
			for (const auto &[name, func] : lak::registered_tests())
			{
				lak::debugger.std_out(lak::u8string(),
				                      lak::to_u8string(name) + u8"\n");
			}
		}
		else if (argv[arg] == lak::astring("-laktestall"))
		{
			return lak::optional<int>(lak::run_tests());
		}
		else if (argv[arg] == lak::astring("-laktests") ||
		         argv[arg] == lak::astring("-laktest"))
		{
			++arg;
			if (arg >= argc) FATAL("Missing tests");
			return lak::optional<int>(lak::run_tests(
			  lak::as_u8string(lak::astring_view::from_c_str(argv[arg]))));
		}
		else if (argv[arg] == lak::astring("-test"))
		{
			se_main_mode = se_main_mode_t::testing;
		}
		else
		{
			SrcExp.baby_mode   = false;
			SrcExp.exe.path    = argv[arg];
			SrcExp.exe.valid   = true;
			SrcExp.exe.attempt = true;
			if (!lak::path_exists(SrcExp.exe.path).UNWRAP())
				FATAL(SrcExp.exe.path, " does not exist");
		}
	}

	basic_window_target_framerate      = 30;
	basic_window_opengl_settings.major = 3;
	basic_window_opengl_settings.minor = 2;
	basic_window_clear_colour          = {0.0f, 0.0f, 0.0f, 1.0f};

	return lak::nullopt;
}

void basic_window_init(lak::window &window)
{
	lak::debugger.crash_path = SrcExp.error_log.path =
	  fs::current_path() / "ATTACH-TO-ISSUE-ON-SOURCE-EXPLORER-GITHUB-REPO.txt";

	SrcExp.images.path = SrcExp.sorted_images.path = SrcExp.sounds.path =
	  SrcExp.music.path = SrcExp.shaders.path = SrcExp.binary_files.path =
	    SrcExp.appicon.path = SrcExp.binary_block.path = fs::current_path();

	SrcExp.testing.path = fs::current_path() / "test";

	lak::debugger.live_output_enabled = true;

	if (!SrcExp.exe.attempt)
	{
		lak::debugger.live_errors_only = true;
		SrcExp.exe.path                = fs::current_path();
	}
	else
	{
		lak::debugger.live_errors_only = force_only_error;
	}

	SrcExp.graphics_mode = window.graphics();
	imgui_context        = ImGui::ImplCreateContext(SrcExp.graphics_mode);
	ImGui::ImplInit();
	ImGui::ImplInitContext(imgui_context, window);

	DEBUG("Graphics: ", SrcExp.graphics_mode);
	if (!lak::debugger.live_output_enabled || lak::debugger.live_errors_only)
		std::cout << "Graphics: " << SrcExp.graphics_mode << "\n";

	switch (SrcExp.graphics_mode)
	{
		case lak::graphics_mode::OpenGL:
		{
			opengl_major = lak::opengl::get_uint(GL_MAJOR_VERSION);
			opengl_minor = lak::opengl::get_uint(GL_MINOR_VERSION);
		}
		break;

		case lak::graphics_mode::Software:
		{
			ImGuiStyle &style      = ImGui::GetStyle();
			style.AntiAliasedLines = false;
			style.AntiAliasedFill  = false;
			style.WindowRounding   = 0.0f;
		}
		break;

		default: break;
	}

#ifdef LAK_USE_SDL
	if (SDL_Init(SDL_INIT_AUDIO)) ERROR("Failed to initialise SDL audio");
#endif

	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();
	ImGui::GetStyle().WindowRounding = 0;
}

void basic_window_handle_event(lak::window &, lak::event &event)
{
	ImGui::ImplProcessEvent(imgui_context, event);

	switch (event.type)
	{
		case lak::event_type::dropfile:
			SrcExp.exe.path    = event.dropfile().path;
			SrcExp.exe.valid   = true;
			SrcExp.exe.attempt = true;
			break;
		default: break;
	}
}

void basic_window_loop(lak::window &window, uint64_t counter_delta)
{
	const float frame_time = (float)counter_delta / lak::performance_frequency();
	ImGui::ImplNewFrame(imgui_context, window, frame_time);

	bool mainOpen = true;

	ImGuiStyle &style = ImGui::GetStyle();
	ImGuiIO &io       = ImGui::GetIO();

	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(io.DisplaySize);
	ImVec2 old_window_padding = style.WindowPadding;
	style.WindowPadding       = ImVec2(0.0f, 0.0f);
	if (ImGui::Begin(APP_NAME,
	                 &mainOpen,
	                 ImGuiWindowFlags_AlwaysAutoResize |
	                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar |
	                   ImGuiWindowFlags_NoSavedSettings |
	                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove))
	{
		style.WindowPadding = old_window_padding;
		MainScreen(frame_time);
		ImGui::End();
	}

	ImGui::ImplRender(imgui_context);
}

int basic_window_quit(lak::window &)
{
	ImGui::ImplShutdownContext(imgui_context);
	return 0;
}
