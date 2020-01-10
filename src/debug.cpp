/*
MIT License

Copyright (c) 2019, 2020 LAK132

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

#include "debug.h"

#include <fstream>

namespace lak
{
  void debugger_t::std_out(const std::string &line_info, const std::string &str)
  {
    stream << scoped_indenter::str() << line_info << str;
    if (live_output_enabled && !live_errors_only)
    {
      std::cout << scoped_indenter::str();
      if (line_info_enabled)
      {
        std::cout << line_info;
      }
      std::cout << str << std::flush;
    }
  }

  void debugger_t::std_err(const std::string &line_info, const std::string &str)
  {
    stream << scoped_indenter::str() << line_info << str;
    if (live_output_enabled)
    {
      std::cerr << scoped_indenter::str();
      if (line_info_enabled)
      {
        std::cerr << line_info;
      }
      std::cerr << str << std::flush;
    }
  }

  void debugger_t::clear()
  {
    stream.clear();
  }

  std::string debugger_t::str()
  {
    return stream.str();
  }

  void debugger_t::abort()
  {
    std::cerr << "Something went wrong! Aborting...\n";
    if (!crash_path.empty())
      std::cerr << "Saving crash log to '" << lak::debugger.save() << "'.\n";
    std::cerr << "Please forward the crash log onto the developer so they can "
                 "attempt to fix the issues that caused this crash.\n";
    std::cerr << "Press enter to continue...\n";
    getchar();
    std::abort();
  }


  std::filesystem::path debugger_t::save()
  {
    return save(crash_path);
  }

  std::filesystem::path debugger_t::save(const std::filesystem::path &path)
  {
    if (!path.string().empty())
    {
      std::ofstream file(path, std::ios::out | std::ios::trunc);
      if (file.is_open()) file << str();
    }

    std::error_code ec;
    if (auto absolute = std::filesystem::absolute(path, ec); ec)
      return path;
    else
      return absolute;
  }

  scoped_indenter::scoped_indenter(const std::string &name)
  {
    debugger.std_out("", name + " {\n");
    ++debug_indent;
  }

  scoped_indenter::~scoped_indenter()
  {
    --debug_indent;
    debugger.std_out("", "}\n");
  }

  std::string scoped_indenter::str()
  {
    std::string s;
    for (size_t i = debug_indent; i --> 0;)
      s += ((debug_indent - i) & 1) ? "| " : ": ";
    return s;
  }

  std::wstring scoped_indenter::wstr()
  {
    std::wstring s;
    for (size_t i = debug_indent; i --> 0;)
      s += ((debug_indent - i) & 1) ? L"| " : L": ";
    return s;
  }

  size_t debug_indent;

  debugger_t debugger;
}