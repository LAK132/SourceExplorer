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

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <lak/debug.hpp>

#include <algorithm>
#include <deque>

namespace lak
{
  bool input_text(const char *str_id,
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

  bool input_text(const char *str_id,
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
    fs::path folder;
    bool folder_has_parent;
    fs::path folder_parent;
    fs::path file;

    lak::result<bool, std::error_code> refresh(const fs::path &path)
    {
      if (auto norm = lak::normalised(path); norm != full())
      {
        auto deepest = lak::deepest_folder(norm);
        if (deepest.is_err()) return lak::err_t{deepest.unwrap_err()};
        const auto [_folder, _file] = deepest.unwrap();

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

    fs::path full() { return lak::normalised(folder / file); }
  };

  bool path_navigator(fs::path &path, std::error_code &ec, ImVec2 size)
  {
    static path_cache cache;
    static std::vector<fs::path> folders;
    static std::vector<fs::path> files;

    if (path.empty()) path = fs::current_path();
    if (auto refreshed = cache.refresh(path); refreshed.is_err())
    {
      ec = refreshed.unwrap_err();
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
        const auto str = folder.filename().u8string() + '/';
        if (ImGui::Selectable(str.c_str()))
        {
          // User selected a folder
          path = folder / cache.file;
        }
      }

      ImGui::Separator();

      for (const auto &file : files)
      {
        if (ImGui::Selectable(file.filename().u8string().c_str()))
        {
          // User selected a file
          path = file;
        }
      }
    }
    ImGui::EndChild();

    return path != cache.full();
  }

  lak::result<file_open_error, std::error_code> open_file(fs::path &path,
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
      file_str   = cache.file.u8string();
      folder_str = cache.folder.u8string();
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
          ec = result.unwrap_err();
        path       = cache.full();
        file_str   = cache.file.u8string();
        folder_str = cache.folder.u8string();
      }
      if (ec) return lak::err_t{ec};

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

  lak::result<file_open_error, std::error_code> open_folder(fs::path &path,
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
      folder_str = cache.folder.u8string();
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
          ec = result.unwrap_err();
        path       = cache.full();
        folder_str = cache.folder.u8string();
      }
      if (ec) return lak::err_t{ec};

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

  lak::result<file_open_error, std::error_code> open_file_modal(fs::path &path,
                                                                bool save)
  {
    const char *name = save ? "Save File" : "Open File";

    ImGui::SetNextWindowSizeConstraints({300, 500}, {1000, 14000});
    if (ImGui::BeginPopupModal(
          name, nullptr, ImGuiWindowFlags_NoSavedSettings))
    {
      DEFER(ImGui::EndPopup());
      auto result = open_file(path, save);
      if (!(result.is_ok() && result.unwrap() == file_open_error::INCOMPLETE))
      {
        ImGui::CloseCurrentPopup();
        return result;
      }
    }

    if (!ImGui::IsPopupOpen(name)) ImGui::OpenPopup(name);

    return lak::ok_t{file_open_error::INCOMPLETE};
  }

  lak::result<file_open_error, std::error_code> open_folder_modal(
    fs::path &path)
  {
    const char name[] = "Open Folder";

    ImGui::SetNextWindowSizeConstraints({300, 500}, {1000, 14000});
    if (ImGui::BeginPopupModal(
          name, nullptr, ImGuiWindowFlags_NoSavedSettings))
    {
      DEFER(ImGui::EndPopup());
      auto result = open_folder(path);
      if (!(result.is_ok() && result.unwrap() == file_open_error::INCOMPLETE))
      {
        ImGui::CloseCurrentPopup();
        return result;
      }
    }

    if (!ImGui::IsPopupOpen(name)) ImGui::OpenPopup(name);

    return lak::ok_t{file_open_error::INCOMPLETE};
  }
}
