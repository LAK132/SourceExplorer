/*
MIT License

Copyright (c) 2019 LAK132

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "imgui_utils.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include <lak/debug.hpp>
#include <lak/defer.hpp>
#include <lak/file.hpp>

#include <lak/imgui/widgets.hpp>

#include <algorithm>
#include <deque>

#include <misc/cpp/imgui_stdlib.cpp>

bool lak::input_text(const char *str_id,
                     char *buf,
                     size_t buf_size,
                     ImGuiInputTextFlags flags,
                     ImGuiInputTextCallback callback,
                     void *user_data)
{
	bool result;
	ImGui::PushID(str_id);
	result = ImGui::InputTextEx(
	  "", "", buf, (int)buf_size, ImVec2(-1, 0), flags, callback, user_data);
	ImGui::PopID();
	return result;
}

bool lak::input_text(const char *str_id,
                     lak::astring *str,
                     ImGuiInputTextFlags flags,
                     ImGuiInputTextCallback callback,
                     void *user_data)
{
	IM_ASSERT(!(flags & ImGuiInputTextFlags_CallbackResize));
	flags |= ImGuiInputTextFlags_CallbackResize;

	InputTextCallback_UserData cb_user_data;
	cb_user_data.Str                   = str;
	cb_user_data.ChainCallback         = callback;
	cb_user_data.ChainCallbackUserData = user_data;

	return lak::input_text(str_id,
	                       (char *)str->c_str(),
	                       str->capacity() + 1,
	                       flags,
	                       InputTextCallback,
	                       &cb_user_data);
}

struct path_cache
{
	lak::fs::path folder;
	bool folder_has_parent;
	lak::fs::path folder_parent;
	lak::fs::path file;

	lak::error_code_result<bool> refresh(const lak::fs::path &path)
	{
		if (auto norm = lak::normalised(path); norm != full())
		{
			auto deepest = lak::deepest_folder(norm);
			if (deepest.is_err()) return lak::err_t{deepest.unsafe_unwrap_err()};
			const auto [_folder, _file] = deepest.unsafe_unwrap();

			file = _file;

			if (_folder != folder)
			{
				folder            = _folder;
				folder_has_parent = lak::has_parent(folder);
				folder_parent     = lak::parent(folder);
				return lak::ok_t{true};
			}
		}

		return lak::ok_t{false};
	}

	lak::fs::path full() { return lak::normalised(folder / file); }
};

bool lak::path_navigator(fs::path &path, std::error_code &ec, ImVec2 size)
{
	static path_cache cache;
	static std::vector<fs::path> folders;
	static std::vector<fs::path> files;

	if (path.empty()) path = fs::current_path();
	if (auto refreshed = cache.refresh(path); refreshed.is_err())
	{
		ec = refreshed.unsafe_unwrap_err().value;
		return false;
	}
	else if (refreshed.unwrap())
	{
		// Folder path changed

		std::deque<fs::path> temp_folders;
		std::deque<fs::path> temp_files;

		for (const auto &entry : fs::directory_iterator(cache.folder))
		{
			if (entry.is_directory())
				temp_folders.emplace_back(entry);
			else
				temp_files.emplace_back(entry);
		}

		folders = {temp_folders.begin(), temp_folders.end()};
		files   = {temp_files.begin(), temp_files.end()};

		std::sort(folders.begin(), folders.end());
		std::sort(files.begin(), files.end());
	}

	if (ImGui::BeginChild(
	      "##path_navigator", size, false, ImGuiWindowFlags_NoSavedSettings))
	{
		if (ImGui::Selectable("../"))
		{
			path = lak::normalised(lak::parent(cache.folder) / cache.file);
		}

		ImGui::Separator();

		for (const auto &folder : folders)
		{
			const auto str = lak::to_u8string(folder.filename()) + u8'/';
			if (ImGui::Selectable(reinterpret_cast<const char *>(str.c_str())))
			{
				// User selected a folder
				path = folder / cache.file;
			}
		}

		ImGui::Separator();

		for (const auto &file : files)
		{
			if (ImGui::Selectable(reinterpret_cast<const char *>(
			      lak::to_u8string(file.filename()).c_str())))
			{
				// User selected a file
				path = file;
			}
		}
	}
	ImGui::EndChild();

	return path != cache.full();
}

lak::error_code_result<lak::file_open_error> lak::open_file(fs::path &path,
                                                            bool save,
                                                            ImVec2 size)
{
	static path_cache cache;
	static lak::astring file_str;
	static lak::astring folder_str;
	if (path.empty()) path = fs::current_path();

	if (auto refreshed = cache.refresh(path); refreshed.is_err())
	{
		return lak::err_t{refreshed.unwrap_err()};
	}
	else if (refreshed.unwrap())
	{
		file_str   = lak::as_astring(cache.file.u8string()).to_string();
		folder_str = lak::as_astring(cache.folder.u8string()).to_string();
	}

	if (DEFER(ImGui::EndChild()); ImGui::BeginChild(
	      "##open_file", size, true, ImGuiWindowFlags_NoSavedSettings))
	{
		lak::input_text("Directory", &folder_str);
		if (ImGui::IsItemDeactivatedAfterEdit()) path = folder_str / cache.file;

		ImGui::Separator();

		const ImVec2 viewSize = ImVec2(0,
		                               -((ImGui::GetStyle().ItemSpacing.y +
		                                  ImGui::GetFrameHeightWithSpacing()) *
		                                 2));

		std::error_code ec;
		if (auto full = cache.full(); path_navigator(full, ec, viewSize))
		{
			if (auto result = cache.refresh(full); result.is_err())
				ec = result.unsafe_unwrap_err().value;
			path       = cache.full();
			file_str   = lak::as_astring(lak::to_u8string(cache.file)).to_string();
			folder_str = lak::as_astring(lak::to_u8string(cache.folder)).to_string();
		}
		if (ec) return lak::err_t<lak::error_code_error>{ec};

		ImGui::Separator();

		lak::input_text("File", &file_str);
		if (ImGui::IsItemDeactivatedAfterEdit()) path = cache.folder / file_str;

		ImGui::Separator();

		if (fs::is_directory(cache.folder) && !cache.file.filename().empty())
		{
			if (save)
			{
				if (ImGui::Button("Save"))
				{
					path = cache.full();
					return lak::ok_t{file_open_error::VALID};
				}

				ImGui::SameLine();
			}
			else if (fs::is_regular_file(cache.full()))
			{
				if (ImGui::Button("Open"))
				{
					path = cache.full();
					return lak::ok_t{file_open_error::VALID};
				}

				ImGui::SameLine();
			}
		}

		if (ImGui::Button("Cancel"))
		{
			return lak::ok_t{file_open_error::CANCELED};
		}
	}

	return lak::ok_t{file_open_error::INCOMPLETE};
}

lak::error_code_result<lak::file_open_error> lak::open_folder(fs::path &path,
                                                              ImVec2 size)
{
	static path_cache cache;
	static lak::astring folder_str;
	if (path.empty()) path = fs::current_path();
	if (auto refreshed = cache.refresh(path); refreshed.is_err())
	{
		return lak::err_t{refreshed.unwrap_err()};
	}
	else if (refreshed.unwrap())
	{
		folder_str = lak::as_astring(lak::to_u8string(cache.folder)).to_string();
	}

	if (DEFER(ImGui::EndChild()); ImGui::BeginChild(
	      "##open_folder", size, true, ImGuiWindowFlags_NoSavedSettings))
	{
		lak::input_text("Directory", &folder_str);
		if (ImGui::IsItemDeactivatedAfterEdit()) path = folder_str / cache.file;

		ImGui::Separator();

		const ImVec2 viewSize = ImVec2(0,
		                               -((ImGui::GetStyle().ItemSpacing.y +
		                                  ImGui::GetFrameHeightWithSpacing()) *
		                                 2));
		std::error_code ec;
		if (auto full = cache.full(); path_navigator(full, ec, viewSize))
		{
			if (auto result = cache.refresh(full); result.is_err())
				ec = result.unsafe_unwrap_err().value;
			cache.file.clear();
			path       = cache.full();
			folder_str = lak::as_astring(lak::to_u8string(cache.folder)).to_string();
		}
		if (ec) return lak::err_t<lak::error_code_error>{ec};

		ImGui::Separator();

		if (fs::is_directory(cache.full()))
		{
			if (ImGui::Button("Open"))
			{
				path = cache.full();
				return lak::ok_t{file_open_error::VALID};
			}

			ImGui::SameLine();
		}

		if (ImGui::Button("Cancel"))
		{
			return lak::ok_t{file_open_error::CANCELED};
		}
	}

	return lak::ok_t{file_open_error::INCOMPLETE};
}

lak::error_code_result<lak::file_open_error> lak::open_file_modal(
  fs::path &path, bool save, const std::string &filter)
{
	const std::string name = save ? "Save File" : "Open File";

	if (!ImGui::IsPopupOpen((name + "###" + name).c_str()))
	{
		if (save)
		{
			if (!lak::file_dialog->Save(name, name, filter, path.string()))
				ASSERT_UNREACHABLE();
		}
		else
		{
			if (!lak::file_dialog->Open(name, name, filter, false, path.string()))
				ASSERT_UNREACHABLE();
		}
	}

	if (lak::file_dialog->IsDone(name))
	{
		const bool has_result = lak::file_dialog->HasResult();
		if (has_result) path = lak::file_dialog->GetResult();
		lak::file_dialog->Close();
		return lak::ok_t{has_result ? lak::file_open_error::VALID
		                            : lak::file_open_error::CANCELED};
	}

	return lak::ok_t{lak::file_open_error::INCOMPLETE};
}

lak::error_code_result<lak::file_open_error> lak::open_folder_modal(
  fs::path &path)
{
	const std::string name = "Open Folder";

	if (!ImGui::IsPopupOpen((name + "###" + name).c_str()))
		if (!lak::file_dialog->Open(name, name, "", false, path.string()))
			ASSERT_UNREACHABLE();

	if (lak::file_dialog->IsDone(name))
	{
		const bool has_result = lak::file_dialog->HasResult();
		if (has_result) path = lak::file_dialog->GetResult();
		lak::file_dialog->Close();
		return lak::ok_t{has_result ? lak::file_open_error::VALID
		                            : lak::file_open_error::CANCELED};
	}

	return lak::ok_t{lak::file_open_error::INCOMPLETE};
}
