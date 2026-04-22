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
#include <SDL.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_utils.hpp"

#include "dump.h"
#include "main.h"

#include "main_window.hpp"

#include <lak/bank_ptr.hpp>
#include <lak/defer.hpp>
#include <lak/string_literals/string.hpp>
#include <lak/string_utils.hpp>
#include <lak/system/file.hpp>
#include <lak/system/windowing/window.hpp>
#include <lak/test.hpp>

#ifndef MAXDIRLEN
#	define MAXDIRLEN 512
#endif

srcexp::instance_t *SrcExp;
lak::optional<srcexp::file_state_t> initial_file_state;

#define LAK_BASIC_PROGRAM_IMGUI_WINDOW_IMPL
#include <lak/basic_program.inl>

bool init_force_only_error = false;
srcexp::instance_t::main_mode_t init_main_mode =
  srcexp::instance_t::main_mode_t::normal;
bool init_allow_multithreading = false;

struct my_window : virtual public basic_window_api
{
	my_window() : basic_window_api() {}

	main_window srcexp_window;

	virtual void init() override final
	{
		window().set_title(L"" APP_NAME);

		srcexp_window.srcexp_instance.error_log.path =
		  fs::current_path() /
		  "ATTACH-TO-ISSUE-ON-SOURCE-EXPLORER-GITHUB-REPO.txt";

		srcexp_window.srcexp_instance.images.path =
		  srcexp_window.srcexp_instance.sorted_images.path =
		    srcexp_window.srcexp_instance.sounds.path =
		      srcexp_window.srcexp_instance.music.path =
		        srcexp_window.srcexp_instance.shaders.path =
		          srcexp_window.srcexp_instance.binary_files.path =
		            srcexp_window.srcexp_instance.appicon.path =
		              srcexp_window.srcexp_instance.binary_block.path =
		                fs::current_path();

		srcexp_window.srcexp_instance.testing.path = fs::current_path() / "test";
	}

	virtual ~my_window() {}

	virtual void handle_event(lak::event &event) override final
	{
		switch (event.type)
		{
			case lak::event_type::close_window: destroy(); break;

			case lak::event_type::dropfile:
			{
				srcexp_window.srcexp_instance.exe.path    = event.dropfile().path;
				srcexp_window.srcexp_instance.exe.valid   = true;
				srcexp_window.srcexp_instance.exe.attempt = true;
			}
			break;

			default: break;
		}
	}

	virtual void loop(uint64_t counter_delta) override final
	{
		const float frame_time =
		  (float)counter_delta / lak::performance_frequency();

		SrcExp = &srcexp_window.srcexp_instance;
		srcexp_window.draw(frame_time);
	}
};

lak::graphics_mode forced_graphics_mode = lak::graphics_mode::None;

lak::error_code<int> basic_program_preinit(lak::span<char *> args)
{
	if (args.size() == 2 && args[1] == "--version"_str)
	{
		std::cout << "Source Explorer " APP_VERSION << "\n";
		return lak::err_t{EXIT_SUCCESS};
	}
	else if (args.size() == 2 && args[1] == "--full-version"_str)
	{
		std::cout << APP_NAME << "\n";
		return lak::err_t{EXIT_SUCCESS};
	}

	lak::debugger.std_out(u8"", u8"" APP_NAME "\n");

	for (size_t arg = 1U; arg < args.size(); ++arg)
	{
		if (args[arg] == "-h"_str || args[arg] == "--help"_str)
		{
			std::cout << "srcexp.exe "
			             "[--help] "
			             "[--software | --opengl] "
			             "[--noisy] "
			             "[--onlyerr] "
			             "[--listtests | --laktestall | --laktests \"test1;test2\"] "
			             "[--test] [--skip-broken] [--open-broken] [--threaded] "
			             "[--analyse] "
			             "[--lua] "
			             "[<filepath>]\n";
			return lak::err_t{EXIT_SUCCESS};
		}
		else if (args[arg] == "--software"_str)
		{
			forced_graphics_mode = lak::graphics_mode::Software;
		}
		else if (args[arg] == "--opengl"_str)
		{
			forced_graphics_mode = lak::graphics_mode::OpenGL;
		}
		else if (args[arg] == "--onlyerr"_str)
		{
			init_force_only_error = true;
		}
		else if (args[arg] == "--listtests"_str)
		{
			lak::debugger.std_out(lak::u8string(), u8"Available tests:\n"_str);
			for (const auto &[name, func] : lak::registered_tests())
			{
				lak::debugger.std_out(lak::u8string(),
				                      lak::to_u8string(name) + u8"\n");
			}
		}
		else if (args[arg] == "--laktestall"_str)
		{
			return lak::err_t{lak::run_tests()};
		}
		else if (args[arg] == "--laktests"_str || args[arg] == "--laktest"_str)
		{
			++arg;
			if (arg >= args.size()) FATAL("Missing tests");
			return lak::err_t{lak::run_tests(
			  lak::as_u8string(lak::astring_view::from_c_str(args[arg])))};
		}
		else if (args[arg] == "--test"_str)
		{
			init_main_mode = srcexp::instance_t::main_mode_t::testing;
		}
		else if (args[arg] == "--analyse"_str)
		{
			init_main_mode = srcexp::instance_t::main_mode_t::binary_analysis;
		}
		else if (args[arg] == "--skip-broken"_str)
		{
			srcexp::skip_broken_items = true;
		}
		else if (args[arg] == "--open-broken"_str)
		{
			srcexp::open_broken_games = true;
		}
		else if (args[arg] == "--threaded"_str)
		{
			init_allow_multithreading = true;
		}
		else if_let_ok (auto exists,
		                lak::path_exists(lak::fs::path(args[arg]))
		                  .IF_ERR("error checking if path exists"))
		{
			auto p = lak::fs::path(args[arg]);
			if (exists)
			{
				initial_file_state.emplace();
				initial_file_state->path    = p;
				initial_file_state->valid   = true;
				initial_file_state->attempt = true;
			}
			else
			{
				FATAL(p, " does not exist");
			}
		}
	}

	basic_window_target_framerate = 30;

	return lak::ok_t{};
}

lak::array<lak::weak_ptr<LAK_BASIC_PROGRAM(window_instance<my_window>)>>
  my_window_ptrs;

void new_instance_window()
{
	lak::strong_ptr<LAK_BASIC_PROGRAM(window_instance<my_window>)> my_window_ptr;

	auto ref_to_ptr =
	  []<typename T>(lak::strong_ref<T> ref) -> lak::strong_ptr<T>
	{ return ref; };

	switch (forced_graphics_mode)
	{
		case lak::graphics_mode::None:
		{
			my_window_ptr = LAK_BASIC_PROGRAM(create_window<my_window>)()
			                  .IF_ERR()
			                  .map(ref_to_ptr)
			                  .unwrap_or_default();
		}
		break;
#ifdef LAK_ENABLE_SOFTRENDER
		case lak::graphics_mode::Software:
		{
			my_window_ptr = LAK_BASIC_PROGRAM(create_window<my_window>)(
			                  LAK_BASIC_PROGRAM(window_software_settings))
			                  .IF_ERR()
			                  .map(ref_to_ptr)
			                  .unwrap_or_default();
		}
		break;
#endif
#ifdef LAK_ENABLE_OPENGL
		case lak::graphics_mode::OpenGL:
		{
			my_window_ptr = LAK_BASIC_PROGRAM(create_window<my_window>)(
			                  LAK_BASIC_PROGRAM(window_opengl_settings))
			                  .IF_ERR()
			                  .map(ref_to_ptr)
			                  .unwrap_or_default();
		}
		break;
#endif
		default:
			ERROR(
			  lak::fmt<u8"Graphics mode {} not available">(forced_graphics_mode));
	}

	if (my_window_ptr) my_window_ptrs.emplace_back(my_window_ptr);
}

lak::error_code<int> basic_program_init()
{
	new_instance_window();

	auto my_window_ptr = my_window_ptrs.back().get();

	DEBUG_EXPR(my_window_ptr->window().graphics());

	SrcExp = &my_window_ptr->srcexp_window.srcexp_instance;

	lak::debugger.crash_path          = SrcExp->error_log.path;
	lak::debugger.live_output_enabled = true;

	if_let_some (auto file_state, initial_file_state)
	{
		SrcExp->baby_mode = false;
		SrcExp->exe       = lak::move(file_state);
		initial_file_state.reset();
	}

	if (!SrcExp->exe.attempt)
	{
		lak::debugger.live_errors_only = true;
		SrcExp->exe.path = fs::current_path();
	}
	else
	{
		lak::debugger.live_errors_only = init_force_only_error;
	}

	SrcExp->main_mode            = init_main_mode;
	SrcExp->allow_multithreading = init_allow_multithreading;

	return lak::ok_t{};
}

void basic_program_handle_event(lak::event &event)
{
	switch (event.type)
	{
		case lak::event_type::quit_program:
			for (auto &inst : basic_window_instances()) inst->destroy();
			break;
		default: break;
	}
}

bool basic_program_loop(uint64_t counter_delta)
{
	LAK_UNUSED(counter_delta);
	return !basic_window_instances().empty();
}

int basic_program_quit()
{
	my_window_ptrs.clear();
	return EXIT_SUCCESS;
}
