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

#ifndef IMGUI_UTILS_H
#define IMGUI_UTILS_H

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#  define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>

#include <atomic>
#include <filesystem>
#include <thread>

namespace lak
{
  namespace fs = std::filesystem;

  bool input_text(const char *str_id,
                  char *buf,
                  size_t buf_size,
                  ImGuiInputTextFlags flags       = 0,
                  ImGuiInputTextCallback callback = nullptr,
                  void *user_data                 = nullptr);

  bool input_text(const char *str_id,
                  std::string *str,
                  ImGuiInputTextFlags flags       = 0,
                  ImGuiInputTextCallback callback = nullptr,
                  void *user_data                 = nullptr);

  fs::path normalised(const fs::path &path);

  bool has_parent(const fs::path &path);

  // The the ACTUAL parent path.
  // .parent_path() incorrectly changes gets "a/b" from "a/b/", this function
  // would instead return "a/" by normalising to "a/b/."
  fs::path parent(const fs::path &path);

  // Returns {Real folder directory, File part/bad directories}.
  std::pair<fs::path, fs::path> deepest_folder(const fs::path &path,
                                               std::error_code &ec);

  bool path_navigator(fs::path &path,
                      std::error_code &ec,
                      ImVec2 size = {0, 0});

  enum struct file_open_error
  {
    INCOMPLETE,
    INVALID,
    CANCELED,
    VALID
  };

  file_open_error open_file(fs::path &path,
                            bool save,
                            std::error_code &ec,
                            ImVec2 size = {0, 0});

  file_open_error open_folder(fs::path &path,
                              std::error_code &ec,
                              ImVec2 size = {0, 0});

  file_open_error open_file_modal(fs::path &path,
                                  bool save,
                                  std::error_code &ec);

  file_open_error open_folder_modal(fs::path &path, std::error_code &ec);

  template<typename R, typename... T, typename... D>
  bool await_popup(const char *str_id,
                   bool &open,
                   std::unique_ptr<std::thread> &staticThread,
                   std::atomic<bool> &staticFinished,
                   R (*callback)(T...),
                   const std::tuple<D...> &callbackData)
  {
    if (ImGui::BeginPopup(str_id, ImGuiWindowFlags_AlwaysAutoResize))
    {
      if (lak::await(staticThread, staticFinished, callback, callbackData))
      {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        open = false;
        return false;
      }
      open = true;
    }
    else
    {
      open = false;
      ImGui::OpenPopup(str_id);
    }

    return true;
  }

}

#endif