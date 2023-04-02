#ifndef TESTING_WINDOW_HPP
#define TESTING_WINDOW_HPP

#include "base_window.hpp"

#include "main_window.hpp"

#include <lak/strcast.hpp>

struct test_window : public base_window<test_window>
{
	static void menu_bar(float)
	{
		SrcExp.testing.attempt |= ImGui::Button("Open Folder");
		mode_select_menu();
		main_window::compat_menu();
		debug_menu();
	}

	static bool refresh_testing_files()
	{
		SrcExp.testing_files.clear();

		std::error_code ec;
		for (const auto &entry :
		     fs::recursive_directory_iterator{SrcExp.testing.path, ec})
			if (entry.is_regular_file(ec))
				if (const auto path{entry.path()}, extension{path.extension()};
				    extension == ".exe" || extension == ".EXE" ||
				    extension == ".dat" || extension == ".DAT" ||
				    extension == ".ccn" || extension == ".CCN" ||
				    extension == ".gam" || extension == ".GAM" ||
				    extension == ".ugh" || extension == ".UGH")
					SrcExp.testing_files.push_back(path);

		if (ec) ERROR(ec);

		return bool(ec);
	}

	static void main_region()
	{
		if (SrcExp.testing.attempt || !SrcExp.testing.valid)
		{
			SrcExp.testing.attempt = true;
			SrcExp.testing.valid   = false;

			se::Attempt(SrcExp.testing, &se::FolderLoader, &refresh_testing_files);

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

		if (ImGui::Button("Refresh Folder")) refresh_testing_files();

		ImGui::SameLine();

		if (ImGui::Button("Try Open All"))
		{
			all_testing_files   = SrcExp.testing_files;
			testing_files_count = all_testing_files.size();
		}

		for (const auto &path : SrcExp.testing_files)
		{
			LAK_TREE_NODE(lak::as_astring(
			  path.lexically_relative(SrcExp.testing.path).u8string().c_str()))
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

		if (SrcExp.loaded)
		{
			if (SrcExp.loaded_successfully)
			{
				ImGui::Text("\"%s\" Successfully Loaded",
				            lak::as_astring(SrcExp.exe.path.u8string().c_str()));
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Text, 0xFF8080FF);
				ImGui::Text("Loading \"%s\" Failed, Showing Partial Load",
				            lak::as_astring(SrcExp.exe.path.u8string().c_str()));
				ImGui::PopStyleColor();
			}

			if (ImGui::Button("Try Dump Images"))
			{
				SrcExp.images.path = SrcExp.testing.path / "test-image-dump";
				lak::remove_path(SrcExp.images.path)
				  .IF_ERR("Failed To Delete Folder ", SrcExp.images.path);
				if (lak::create_directory(SrcExp.images.path)
				      .IF_ERR("Failed To Create Folder ", SrcExp.images.path)
				      .is_ok())
				{
					DEBUG("Saving Images To ", SrcExp.images.path);
					SrcExp.images.attempt = true;
					SrcExp.images.valid   = true;
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Try Dump Sounds"))
			{
				SrcExp.sounds.path = SrcExp.testing.path / "test-sound-dump";
				lak::remove_path(SrcExp.sounds.path)
				  .IF_ERR("Failed To Delete Folder ", SrcExp.sounds.path);
				if (lak::create_directory(SrcExp.sounds.path)
				      .IF_ERR("Failed To Create Folder ", SrcExp.sounds.path)
				      .is_ok())
				{
					DEBUG("Saving Images To ", SrcExp.sounds.path);
					SrcExp.sounds.attempt = true;
					SrcExp.sounds.valid   = true;
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Try Dump Music"))
			{
				SrcExp.music.path = SrcExp.testing.path / "test-music-dump";
				lak::remove_path(SrcExp.music.path)
				  .IF_ERR("Failed To Delete Folder ", SrcExp.music.path);
				if (lak::create_directory(SrcExp.music.path)
				      .IF_ERR("Failed To Create Folder ", SrcExp.music.path)
				      .is_ok())
				{
					DEBUG("Saving Images To ", SrcExp.music.path);
					SrcExp.music.attempt = true;
					SrcExp.music.valid   = true;
				}
			}

			base_window<main_window>::main_region();
		}
		else
		{
			ImGui::Text("Working Path \"%s\"",
			            lak::as_astring(SrcExp.exe.path.u8string().c_str()));

			if (!last_error.empty()) ImGui::Text(last_error.c_str());

			ImGui::Text("Not Loaded");
		}

		if (SrcExp.exe.attempt)
		{
			SrcExp.loaded              = false;
			SrcExp.loaded_successfully = false;

			if (auto result{se::OpenGame(SrcExp)}; result.is_err())
			{
				if (result.unwrap_err() == lak::await_error::running)
				{
					ImGui::Text("Loading \"%s\"",
					            lak::as_astring(SrcExp.exe.path.u8string().c_str()));

					if (testing_files_count > 0U)
					{
						ImGui::ProgressBar(
						  float(double(testing_files_count - all_testing_files.size()) /
						        testing_files_count));
					}
				}
				else
				{
					// this may happen if an exception was thrown
					ERROR("OpenGame failed");
					SrcExp.exe.attempt = false;
				}
			}
			else if (result.unsafe_unwrap().is_err())
			{
				result.unsafe_unwrap().IF_ERR("OpenGame failed").discard();
				last_error = lak::as_astring(
				  lak::streamify("Opening \"",
				                 SrcExp.exe.path.u8string(),
				                 "\" failed: ",
				                 result.unsafe_unwrap().unsafe_unwrap_err()));

				const bool is_known_bad_game = SrcExp.state.ccn;

				if (!is_known_bad_game) all_testing_files.clear();

				SrcExp.loaded              = se::open_broken_games;
				SrcExp.loaded_successfully = false;
				SrcExp.exe.attempt         = false;
				SrcExp.exe.valid           = false;
			}
			else
			{
				last_error.clear();

				SrcExp.loaded              = true;
				SrcExp.loaded_successfully = true;
				SrcExp.exe.attempt         = false;
				SrcExp.exe.valid           = false;
			}
		}

		if (SrcExp.images.attempt) se::AttemptImages(SrcExp);
		if (SrcExp.sounds.attempt) se::AttemptSounds(SrcExp);
		if (SrcExp.music.attempt) se::AttemptMusic(SrcExp);
	}
};

#endif
