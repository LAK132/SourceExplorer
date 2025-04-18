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

#include "binary_analysis_window.hpp"
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

struct instance_window
{
	lak::unique_ptr<srcexp::instance_t> instance;
	lak::window_handle *window;
};

lak::vector<instance_window> instances;
srcexp::instance_t *SrcExp;
int opengl_major, opengl_minor;

lak::result<instance_window &> find_window_instance(lak::window_handle *window)
{
	for (auto &inst : instances)
		if (inst.window == window) return lak::ok_t<instance_window &>{inst};
	return lak::err_t{};
}

#if 1
void MainScreen(instance_window &inst, float frame_time)
{
	switch (inst.instance->main_mode)
	{
		case srcexp::instance_t::main_mode_t::byte_pairs:
			byte_pairs_window::draw(frame_time);
			break;

		case srcexp::instance_t::main_mode_t::binary_analysis:
			binary_analysis_window::draw(frame_time);
			break;

		case srcexp::instance_t::main_mode_t::testing:
			test_window::draw(frame_time);
			break;

		case srcexp::instance_t::main_mode_t::normal:
			[[fallthrough]];
		default:
			main_window::draw(frame_time);
			break;
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

#define LAK_BASIC_PROGRAM_IMGUI_WINDOW_IMPL
#include <lak/basic_program.inl>

bool init_force_only_error = false;
srcexp::instance_t::main_mode_t init_main_mode =
  srcexp::instance_t::main_mode_t::normal;
bool init_allow_multithreading = false;

lak::optional<int> basic_program_preinit(int argc, char **argv)
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
		if (argv[arg] == lak::astring("-h") || argv[arg] == lak::astring("--help"))
		{
			std::cout << "srcexp.exe [--help] [--nogl] [--onlyerr] "
			             "[--listtests | --laktestall | --laktests \"test1;test2\"] "
			             "[--test] [--skip-broken] [--open-broken] [--threaded] "
			             "[--analyse] [<filepath>]\n";
			return lak::optional<int>(0);
		}
		else if (argv[arg] == lak::astring("--nogl"))
		{
			basic_window_force_software = true;
		}
		else if (argv[arg] == lak::astring("--onlyerr"))
		{
			init_force_only_error = true;
		}
		else if (argv[arg] == lak::astring("--listtests"))
		{
			lak::debugger.std_out(lak::u8string(),
			                      lak::u8string(u8"Available tests:\n"));
			for (const auto &[name, func] : lak::registered_tests())
			{
				lak::debugger.std_out(lak::u8string(),
				                      lak::to_u8string(name) + u8"\n");
			}
		}
		else if (argv[arg] == lak::astring("--laktestall"))
		{
			return lak::optional<int>(lak::run_tests());
		}
		else if (argv[arg] == lak::astring("--laktests") ||
		         argv[arg] == lak::astring("--laktest"))
		{
			++arg;
			if (arg >= argc) FATAL("Missing tests");
			return lak::optional<int>(lak::run_tests(
			  lak::as_u8string(lak::astring_view::from_c_str(argv[arg]))));
		}
		else if (argv[arg] == lak::astring("--test"))
		{
			init_main_mode = srcexp::instance_t::main_mode_t::testing;
		}
		else if (argv[arg] == lak::astring("--analyse"))
		{
			init_main_mode = srcexp::instance_t::main_mode_t::binary_analysis;
		}
		else if (argv[arg] == lak::astring("--skip-broken"))
		{
			srcexp::skip_broken_items = true;
		}
		else if (argv[arg] == lak::astring("--open-broken"))
		{
			srcexp::open_broken_games = true;
		}
		else if (argv[arg] == lak::astring("--threaded"))
		{
			init_allow_multithreading = true;
		}
		else
		{
			// SrcExp->baby_mode   = false;
			// SrcExp->exe.path    = argv[arg];
			// SrcExp->exe.valid   = true;
			// SrcExp->exe.attempt = true;
			// if (!lak::path_exists(SrcExp->exe.path).UNWRAP())
			// 	FATAL(SrcExp->exe.path, " does not exist");
		}
	}

#ifdef LAK_OS_APPLE
	basic_window_force_software = true;
#endif

	basic_window_target_framerate      = 30;
	basic_window_opengl_settings.major = 3;
	basic_window_opengl_settings.minor = 2;
	basic_window_clear_colour          = {0.0f, 0.0f, 0.0f, 1.0f};
	basic_imgui_main_window_flags =
	  ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar |
	  ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoSavedSettings |
	  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove;

	return lak::nullopt;
}

void new_instance_window() { basic_create_window().UNWRAP(); }

lak::optional<int> basic_program_init()
{
	new_instance_window();

	SrcExp = instances.back().instance.get();

	lak::debugger.crash_path = SrcExp->error_log.path;

	lak::debugger.live_output_enabled = true;

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

	srcexp::instance_t::graphics_mode =
	  lak::window_graphics_mode(instances.back().window);

	DEBUG("Graphics: ", srcexp::instance_t::graphics_mode);
	if (!lak::debugger.live_output_enabled || lak::debugger.live_errors_only)
		std::cout << "Graphics: " << srcexp::instance_t::graphics_mode << "\n";

	switch (srcexp::instance_t::graphics_mode)
	{
		case lak::graphics_mode::OpenGL:
		{
			opengl_major = lak::opengl::get_uint(GL_MAJOR_VERSION).UNWRAP();
			opengl_minor = lak::opengl::get_uint(GL_MINOR_VERSION).UNWRAP();
		}
		break;

		case lak::graphics_mode::Software:
			break;

		default:
			break;
	}

	return lak::nullopt;
}

bool source_explorer_running = true;
bool basic_program_loop(uint64_t counter_delta)
{
	LAK_UNUSED(counter_delta);
	return source_explorer_running && !basic_window_instances.empty();
}

int basic_program_quit() { return EXIT_SUCCESS; }

void basic_window_init(lak::window &window)
{
	auto &inst = instances.push_back(instance_window{
	  .instance = lak::unique_ptr<srcexp::instance_t>::make(),
	  .window   = window.handle(),
	});

	inst.instance->error_log.path =
	  fs::current_path() / "ATTACH-TO-ISSUE-ON-SOURCE-EXPLORER-GITHUB-REPO.txt";

	inst.instance->images.path        = inst.instance->sorted_images.path =
	  inst.instance->sounds.path      = inst.instance->music.path =
	    inst.instance->shaders.path   = inst.instance->binary_files.path =
	      inst.instance->appicon.path = inst.instance->binary_block.path =
	        fs::current_path();

	inst.instance->testing.path = fs::current_path() / "test";
}

void basic_window_handle_event(lak::window *window, lak::event &event)
{
	switch (event.type)
	{
		case lak::event_type::close_window:
			ASSERT(!!window);
			basic_destroy_window(*window);
			break;

		case lak::event_type::quit_program:
			source_explorer_running = false;
			break;

		case lak::event_type::dropfile:
		{
			ASSERT(!!window);
			auto *se =
			  find_window_instance(window->handle()).UNWRAP().instance.get();
			ASSERT(!!se);
			se->exe.path    = event.dropfile().path;
			se->exe.valid   = true;
			se->exe.attempt = true;
		}
		break;

		default:
			break;
	}
}

void basic_window_loop(lak::window &window, uint64_t counter_delta)
{
	auto &inst = find_window_instance(window.handle()).UNWRAP();
	SrcExp     = inst.instance.get();
	MainScreen(inst, (float)counter_delta / lak::performance_frequency());
	SrcExp = nullptr;
}

void basic_window_quit(lak::window &window)
{
	for (auto &inst : instances)
	{
		if (inst.window == window.handle())
		{
			instances.erase(&inst);
			break;
		}
	}
}
