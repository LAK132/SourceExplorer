#ifndef TESTING_WINDOW_HPP
#define TESTING_WINDOW_HPP

#include "base_window.hpp"

#include "main_window.hpp"

#include <lak/strcast.hpp>

struct test_window : public base_window<test_window>
{
	static void menu_bar(float)
	{
		mode_select_menu();
		main_window::compat_menu();
		debug_menu();
	}

	static void main_region()
	{
		if (SrcExp.testing.attempt || !SrcExp.testing.valid)
		{
			auto manip = []
			{
				SrcExp.testing_files.clear();

				std::error_code ec;
				for (const auto &entry :
				     fs::directory_iterator{SrcExp.testing.path, ec})
					if (entry.is_regular_file(ec))
						SrcExp.testing_files.push_back(entry.path());

				if (ec) ERROR(ec);

				return bool(ec);
			};

			SrcExp.testing.attempt = true;
			SrcExp.testing.valid   = false;

			se::Attempt(SrcExp.testing, &se::FolderLoader, manip);

			if (SrcExp.testing.valid)
			{
				SrcExp.testing.attempt = false;
			}
			else if (!SrcExp.testing.attempt)
			{
				// User cancelled
				se_main_mode = se_main_mode_t::normal;
				return;
			}
		}
		else
			base_window::main_region();
	}

	inline static lak::array<fs::path> all_testing_files;
	inline static size_t testing_files_count = 0U;
	inline static lak::astring last_error;

	static void left_region()
	{
		ImGui::Text(lak::as_astring(SrcExp.testing.path.u8string().c_str()));

		if (ImGui::Button("Try Open All"))
		{
			all_testing_files   = SrcExp.testing_files;
			testing_files_count = all_testing_files.size();
		}

		for (const auto &path : SrcExp.testing_files)
		{
			LAK_TREE_NODE(lak::as_astring(path.filename().u8string().c_str()))
			{
				if (ImGui::Button("Try Open"))
				{
					SrcExp.baby_mode   = false;
					SrcExp.exe.path    = path;
					SrcExp.exe.attempt = true;
					SrcExp.exe.valid   = true;
				}
			}
		}
	}

	static void right_region()
	{
		if (!SrcExp.exe.attempt && !all_testing_files.empty())
		{
			SrcExp.baby_mode   = false;
			SrcExp.exe.attempt = true;
			SrcExp.exe.valid   = true;
			SrcExp.exe.path    = lak::move(all_testing_files.back());
			all_testing_files.pop_back();
		}

		if (SrcExp.exe.attempt)
		{
			SrcExp.loaded = false;

			if (auto result{se::OpenGame(SrcExp)}; result.is_err())
			{
				ASSERT(result.unwrap_err() == lak::await_error::running);
				ImGui::Text("Loading \"%s\"",
				            lak::as_astring(SrcExp.exe.path.u8string().c_str()));

				if (testing_files_count > 0U)
				{
					ImGui::ProgressBar(
					  float(double(testing_files_count - all_testing_files.size()) /
					        testing_files_count));
				}
			}
			else if (result.unsafe_unwrap().is_err())
			{
				result.unsafe_unwrap().IF_ERR("OpenGame failed").discard();
				last_error = lak::as_astring(
				  u8"Opening \"" + SrcExp.exe.path.u8string() + u8"\" failed:" +
				  result.unsafe_unwrap().unsafe_unwrap_err().to_string());

				const bool is_known_bad_game = SrcExp.state.ccn;

				if (!is_known_bad_game) all_testing_files.clear();

				SrcExp.exe.attempt = false;
				SrcExp.exe.valid   = false;
			}
			else
			{
				last_error.clear();

				SrcExp.loaded      = true;
				SrcExp.exe.attempt = false;
				SrcExp.exe.valid   = false;
			}
		}

		if (SrcExp.loaded)
		{
			ImGui::Text("\"%s\" Successfully Loaded",
			            lak::as_astring(SrcExp.exe.path.u8string().c_str()));
		}
		else
		{
			ImGui::Text("Working Path \"%s\"",
			            lak::as_astring(SrcExp.exe.path.u8string().c_str()));

			if (!last_error.empty()) ImGui::Text(last_error.c_str());

			ImGui::Text("Not Loaded");
		}
	}
};

#endif
