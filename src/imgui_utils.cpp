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
#include "debug.h"

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <algorithm>
#include <deque>

namespace lak
{
  bool input_text(const char *str_id, char* buf, size_t buf_size,
                  ImGuiInputTextFlags flags, ImGuiInputTextCallback callback,
                  void *user_data)
  {
    IM_ASSERT(!(flags & ImGuiInputTextFlags_Multiline) &&
              "Call InputTextMultiline()");

    bool result;
    ImGui::PushID(str_id);
    result = ImGui::InputTextEx("", "", buf, (int)buf_size, ImVec2(-1,0),
                                flags, callback, user_data);
    ImGui::PopID();
    return result;
  }

  bool input_text(const char *str_id, std::string* str,
                  ImGuiInputTextFlags flags, ImGuiInputTextCallback callback,
                  void *user_data)
  {
    IM_ASSERT(!(flags & ImGuiInputTextFlags_CallbackResize));
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;

    return lak::input_text(str_id, (char*)str->c_str(), str->capacity() + 1,
                           flags, InputTextCallback, &cb_user_data);
  }

  fs::path normalised(const fs::path &path)
  {
    // ("a/b" | "a/b/") -> "a/b/." -> "a/b"
    return (path/".").parent_path();
  }

  bool has_parent(const fs::path &path)
  {
    return normalised(path).has_parent_path();
  }

  fs::path parent(const fs::path &path)
  {
    return normalised(path).parent_path();
  }

  std::pair<fs::path, fs::path>
  deepest_folder(const fs::path &path, std::error_code &ec)
  {
    fs::path folder = normalised(path);
    fs::path file;
    auto entry = fs::directory_entry(path, ec);
    while (!entry.is_directory() && has_parent(folder))
    {
      folder = parent(folder);
      file = path.lexically_relative(folder);
      // we're intentionally ignoring errors if there's still parent
      // directories, but we don't want to ignore the error of the last call
      // to fs::directory_entry.
      ec.clear();
      entry = fs::directory_entry(folder, ec);
    }
    if (ec) WARNING(path << ": " << ec.message());
    return {normalised(folder), normalised(file)};
  }

  struct path_cache
  {
    fs::path folder;
    bool folder_has_parent;
    fs::path folder_parent;
    fs::path file;

    bool refresh(const fs::path &path, std::error_code &ec)
    {
      auto norm = normalised(path);
      if (norm != full())
      {
        const auto [_folder, _file] = deepest_folder(norm, ec);
        if (ec) return false;

        file = _file;

        if (_folder != folder)
        {
          folder = _folder;
          folder_has_parent = has_parent(folder);
          folder_parent = parent(folder);
          return true;
        }
      }

      return false;
    }

    fs::path full() { return normalised(folder/file); }
  };

  bool path_navigator(fs::path &path, std::error_code &ec, ImVec2 size)
  {
    static path_cache cache;
    static std::vector<fs::path> folders;
    static std::vector<fs::path> files;

    if (path.empty()) path = fs::current_path();
    if (cache.refresh(path, ec))
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
      files = {temp_files.begin(), temp_files.end()};

      std::sort(folders.begin(), folders.end());
      std::sort(files.begin(), files.end());
    }
    if (ec) return false;

    if (ImGui::BeginChild("##path_navigator", size, false,
                          ImGuiWindowFlags_NoSavedSettings))
    {
      if (ImGui::Selectable("../"))
      {
        path = parent(cache.folder)/cache.file;
      }

      ImGui::Separator();

      for (const auto &folder : folders)
      {
        const auto str = folder.filename().u8string() + '/';
        if (ImGui::Selectable(str.c_str()))
        {
          // User selected a folder
          path = folder/cache.file;
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

  file_open_error open_file(fs::path &path, bool save, std::error_code &ec,
                            ImVec2 size)
  {
    static path_cache cache;
    static std::string file_str;
    static std::string folder_str;
    if (path.empty()) path = fs::current_path();
    if (cache.refresh(path, ec))
    {
      file_str = cache.file.u8string();
      folder_str = cache.folder.u8string();
    }
    if (ec) return file_open_error::INVALID;
    file_open_error code = file_open_error::INCOMPLETE;

    if (ImGui::BeginChild("##open_file", size, true,
                          ImGuiWindowFlags_NoSavedSettings))
    {
      lak::input_text("Directory", &folder_str);
      if (ImGui::IsItemDeactivatedAfterEdit())
        path = folder_str/cache.file;

      ImGui::Separator();

      const ImVec2 viewSize = ImVec2(0,
        -((ImGui::GetStyle().ItemSpacing.y +
           ImGui::GetFrameHeightWithSpacing()) * 2));
      if (auto full = cache.full(); path_navigator(full, ec, viewSize))
      {
        cache.refresh(full, ec);
        path = cache.full();
        file_str = cache.file.u8string();
        folder_str = cache.folder.u8string();
      }
      if (ec) code = file_open_error::INVALID;

      ImGui::Separator();

      lak::input_text("File", &file_str);
      if (ImGui::IsItemDeactivatedAfterEdit())
        path = cache.folder/file_str;

      ImGui::Separator();

      if (fs::is_directory(cache.folder) && !cache.file.filename().empty())
      {
        if (save)
        {
          if (ImGui::Button("Save"))
          {
            path = cache.full();
            code = file_open_error::VALID;
          }

          ImGui::SameLine();
        }
        else if (fs::is_regular_file(cache.full()))
        {
          if (ImGui::Button("Open"))
          {
            path = cache.full();
            code = file_open_error::VALID;
          }

          ImGui::SameLine();
        }
      }

      if (ImGui::Button("Cancel"))
      {
        code = file_open_error::CANCELED;
      }
    }
    ImGui::EndChild();

    return code;
  }

  file_open_error open_folder(fs::path &path, std::error_code &ec, ImVec2 size)
  {
    static path_cache cache;
    static std::string folder_str;
    if (path.empty()) path = fs::current_path();
    if (cache.refresh(path, ec))
    {
      folder_str = cache.folder.u8string();
    }
    if (ec) return file_open_error::INVALID;
    file_open_error code = file_open_error::INCOMPLETE;

    if (ImGui::BeginChild("##open_folder", size, true,
                          ImGuiWindowFlags_NoSavedSettings))
    {
      lak::input_text("Directory", &folder_str);
      if (ImGui::IsItemDeactivatedAfterEdit())
        path = folder_str/cache.file;

      ImGui::Separator();

      const ImVec2 viewSize = ImVec2(0,
        -((ImGui::GetStyle().ItemSpacing.y +
           ImGui::GetFrameHeightWithSpacing()) * 2));
      if (auto full = cache.full(); path_navigator(full, ec, viewSize))
      {
        cache.refresh(full, ec);
        path = cache.full();
        folder_str = cache.folder.u8string();
      }
      if (ec) code = file_open_error::INVALID;

      ImGui::Separator();

      if (fs::is_directory(cache.full()))
      {
        if (ImGui::Button("Open"))
        {
          path = cache.full();
          code = file_open_error::VALID;
        }

        ImGui::SameLine();
      }

      if (ImGui::Button("Cancel"))
      {
        code = file_open_error::CANCELED;
      }
    }
    ImGui::EndChild();

    return code;
  }


  file_open_error open_file_modal(fs::path &path, bool save,
                                  std::error_code &ec)
  {
    file_open_error code = file_open_error::INCOMPLETE;
    bool open = true;
    const char *name = save ? "Save File" : "Open File";

    ImGui::SetNextWindowSizeConstraints({300, 500}, {1000, 14000});
    if (ImGui::BeginPopupModal(name, &open, ImGuiWindowFlags_NoSavedSettings))
    {
      code = open_file(path, save, ec);
      if (code != file_open_error::INCOMPLETE)
      {
        open = false;
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (open && !ImGui::IsPopupOpen(name)) ImGui::OpenPopup(name);
    return code;
  }

  file_open_error open_folder_modal(fs::path &path, std::error_code &ec)
  {
    file_open_error code = file_open_error::INCOMPLETE;
    bool open = true;
    const char name[] = "Open Folder";

    ImGui::SetNextWindowSizeConstraints({300, 500}, {1000, 14000});
    if (ImGui::BeginPopupModal(name, &open, ImGuiWindowFlags_NoSavedSettings))
    {
      code = open_folder(path, ec);
      if (code != file_open_error::INCOMPLETE)
      {
        open = false;
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (open && !ImGui::IsPopupOpen(name)) ImGui::OpenPopup(name);
    return code;
  }
}
